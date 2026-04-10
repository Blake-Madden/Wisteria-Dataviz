///////////////////////////////////////////////////////////////////////////////
// Name:        insertcandlestickplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertcandlestickplotdlg.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertCandlestickPlotDlg::InsertCandlestickPlotDlg(Canvas* canvas,
                                                       const ReportBuilder* reportBuilder,
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
    void InsertCandlestickPlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();
        CreateAnnotationsPage();
        CreateAxisOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Candlestick Plot"), ID_OPTIONS_SECTION,
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

        auto* dateLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Date:"));
        dateLabel->SetFont(dateLabel->GetFont().Bold());
        varGrid->Add(dateLabel, wxSizerFlags{}.CenterVertical());
        m_dateVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_dateVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_dateVarLabel, wxSizerFlags{}.CenterVertical());

        auto* openLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Open:"));
        openLabel->SetFont(openLabel->GetFont().Bold());
        varGrid->Add(openLabel, wxSizerFlags{}.CenterVertical());
        m_openVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_openVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_openVarLabel, wxSizerFlags{}.CenterVertical());

        auto* highLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"High:"));
        highLabel->SetFont(highLabel->GetFont().Bold());
        varGrid->Add(highLabel, wxSizerFlags{}.CenterVertical());
        m_highVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_highVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_highVarLabel, wxSizerFlags{}.CenterVertical());

        auto* lowLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Low:"));
        lowLabel->SetFont(lowLabel->GetFont().Bold());
        varGrid->Add(lowLabel, wxSizerFlags{}.CenterVertical());
        m_lowVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_lowVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_lowVarLabel, wxSizerFlags{}.CenterVertical());

        auto* closeLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Close:"));
        closeLabel->SetFont(closeLabel->GetFont().Bold());
        varGrid->Add(closeLabel, wxSizerFlags{}.CenterVertical());
        m_closeVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_closeVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_closeVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // plot type
        auto* plotTypeSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        plotTypeSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Plot type:")),
                           wxSizerFlags{}.CenterVertical());
        wxArrayString plotTypes;
        plotTypes.Add(_(L"Candlestick"));
        plotTypes.Add(/* TRANSLATORS: Open/High/Low/Close chart. */ _(L"OHLC"));
        plotTypeSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        plotTypes, 0, wxGenericValidator(&m_plotTypeIndex)),
                           wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(plotTypeSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    void InsertCandlestickPlotDlg::OnDatasetChanged()
        {
        m_dateVariable.clear();
        m_openVariable.clear();
        m_highVariable.clear();
        m_lowVariable.clear();
        m_closeVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertCandlestickPlotDlg::OnSelectVariables()
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
                  .Label(_(L"Date"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_dateVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_dateVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Date }),
              VLI{}
                  .Label(_(L"Open"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_openVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_openVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"High"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_highVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_highVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Low"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_lowVariable.empty() ? std::vector<wxString>{} :
                                                            std::vector<wxString>{ m_lowVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Close"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_closeVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_closeVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto dateVars = dlg.GetSelectedVariables(0);
        m_dateVariable = dateVars.empty() ? wxString{} : dateVars.front();

        const auto openVars = dlg.GetSelectedVariables(1);
        m_openVariable = openVars.empty() ? wxString{} : openVars.front();

        const auto highVars = dlg.GetSelectedVariables(2);
        m_highVariable = highVars.empty() ? wxString{} : highVars.front();

        const auto lowVars = dlg.GetSelectedVariables(3);
        m_lowVariable = lowVars.empty() ? wxString{} : lowVars.front();

        const auto closeVars = dlg.GetSelectedVariables(4);
        m_closeVariable = closeVars.empty() ? wxString{} : closeVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertCandlestickPlotDlg::UpdateVariableLabels()
        {
        m_dateVarLabel->SetLabel(m_dateVariable);
        m_openVarLabel->SetLabel(m_openVariable);
        m_highVarLabel->SetLabel(m_highVariable);
        m_lowVarLabel->SetLabel(m_lowVariable);
        m_closeVarLabel->SetLabel(m_closeVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertCandlestickPlotDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertCandlestickPlotDlg::GetSelectedDataset() const
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
    bool InsertCandlestickPlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_dateVariable.empty() || m_openVariable.empty() || m_highVariable.empty() ||
            m_lowVariable.empty() || m_closeVariable.empty())
            {
            wxMessageBox(_(L"Please select the date, open, high, low, and close variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertCandlestickPlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* candle = dynamic_cast<const Graphs::CandlestickPlot*>(&graph);
        if (candle == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = candle->GetPropertyTemplate(L"dataset");
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
        m_dateVariable = candle->GetPropertyTemplate(L"variables.date");
        m_openVariable = candle->GetPropertyTemplate(L"variables.open");
        m_highVariable = candle->GetPropertyTemplate(L"variables.high");
        m_lowVariable = candle->GetPropertyTemplate(L"variables.low");
        m_closeVariable = candle->GetPropertyTemplate(L"variables.close");
        UpdateVariableLabels();

        // plot type
        m_plotTypeIndex =
            (candle->GetPlotType() == Graphs::CandlestickPlot::PlotType::Ohlc) ? 1 : 0;

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
