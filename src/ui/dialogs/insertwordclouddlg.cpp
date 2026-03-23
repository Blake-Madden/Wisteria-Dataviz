///////////////////////////////////////////////////////////////////////////////
// Name:        insertwordclouddlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertwordclouddlg.h"
#include "../../graphs/wordcloud.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWordCloudDlg::InsertWordCloudDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertWordCloudDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Word Cloud Options"), ID_OPTIONS_SECTION, true);

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

        auto* wordLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Word:"));
        wordLabel->SetFont(wordLabel->GetFont().Bold());
        varGrid->Add(wordLabel, wxSizerFlags{}.CenterVertical());
        m_wordVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_wordVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_wordVarLabel, wxSizerFlags{}.CenterVertical());

        auto* weightLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Weight:"));
        weightLabel->SetFont(weightLabel->GetFont().Bold());
        varGrid->Add(weightLabel, wxSizerFlags{}.CenterVertical());
        m_weightVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_weightVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_weightVarLabel, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(varGrid, wxSizerFlags{}.Border());

        // color scheme
        auto* colorSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Color scheme:")),
                        wxSizerFlags{}.CenterVertical());
        colorSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     GetColorSchemeNames(), 0,
                                     wxGenericValidator(&m_colorSchemeIndex)),
                        wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(colorSizer, wxSizerFlags{}.Border());

        // frequency options
        auto* freqSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        freqSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Minimum frequency:")),
                       wxSizerFlags{}.CenterVertical());
        m_minFreqSpin = new wxSpinCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
        freqSizer->Add(m_minFreqSpin);

        freqSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Maximum frequency:")),
                       wxSizerFlags{}.CenterVertical());
        m_maxFreqSpin = new wxSpinCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
        freqSizer->Add(m_maxFreqSpin);

        freqSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Maximum words:")),
                       wxSizerFlags{}.CenterVertical());
        m_maxWordsSpin = new wxSpinCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                        wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, 0);
        freqSizer->Add(m_maxWordsSpin);

        optionsSizer->Add(freqSizer, wxSizerFlags{}.Border());

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
    void InsertWordCloudDlg::OnDatasetChanged()
        {
        m_wordVariable.clear();
        m_weightVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWordCloudDlg::OnSelectVariables()
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
                  .Label(_(L"Word"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_wordVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_wordVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Weight"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_weightVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_weightVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto wordVars = dlg.GetSelectedVariables(0);
        m_wordVariable = wordVars.empty() ? wxString{} : wordVars.front();

        const auto weightVars = dlg.GetSelectedVariables(1);
        m_weightVariable = weightVars.empty() ? wxString{} : weightVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWordCloudDlg::UpdateVariableLabels()
        {
        m_wordVarLabel->SetLabel(m_wordVariable);
        m_weightVarLabel->SetLabel(m_weightVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertWordCloudDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertWordCloudDlg::GetSelectedDataset() const
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
    size_t InsertWordCloudDlg::GetMinFrequency() const noexcept
        {
        const int val = m_minFreqSpin->GetValue();
        return (val > 0) ? static_cast<size_t>(val) : 1;
        }

    //-------------------------------------------
    std::optional<size_t> InsertWordCloudDlg::GetMaxFrequency() const noexcept
        {
        const int val = m_maxFreqSpin->GetValue();
        return (val > 0) ? std::optional<size_t>(static_cast<size_t>(val)) : std::nullopt;
        }

    //-------------------------------------------
    std::optional<size_t> InsertWordCloudDlg::GetMaxWords() const noexcept
        {
        const int val = m_maxWordsSpin->GetValue();
        return (val > 0) ? std::optional<size_t>(static_cast<size_t>(val)) : std::nullopt;
        }

    //-------------------------------------------
    bool InsertWordCloudDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_wordVariable.empty())
            {
            wxMessageBox(_(L"Please select the word variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertWordCloudDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* wordCloud = dynamic_cast<const Graphs::WordCloud*>(&graph);
        if (wordCloud == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = wordCloud->GetPropertyTemplate(L"dataset");
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
        m_wordVariable = wordCloud->GetWordColumnName();
        m_weightVariable = wordCloud->GetWeightColumnName();
        UpdateVariableLabels();

        // determine which color scheme is in use
        m_colorSchemeIndex = ColorSchemeToIndex(wordCloud->GetColorScheme());

        // frequency options
        m_minFreqSpin->SetValue(static_cast<int>(wordCloud->GetMinFrequency()));
        const auto maxFreq = wordCloud->GetMaxFrequency();
        m_maxFreqSpin->SetValue(maxFreq.has_value() ? static_cast<int>(maxFreq.value()) : 0);
        const auto maxWords = wordCloud->GetMaxWords();
        m_maxWordsSpin->SetValue(maxWords.has_value() ? static_cast<int>(maxWords.value()) : 0);

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
