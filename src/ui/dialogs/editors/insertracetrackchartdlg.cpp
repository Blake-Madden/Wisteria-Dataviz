///////////////////////////////////////////////////////////////////////////////
// Name:        insertracetrackchartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertracetrackchartdlg.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertRaceTrackChartDlg::InsertRaceTrackChartDlg(Canvas* canvas,
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
    void InsertRaceTrackChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Race Track Chart"), ID_OPTIONS_SECTION, true);

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

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Value:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        auto* labelLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Label:"));
        labelLabel->SetFont(labelLabel->GetFont().Bold());
        varGrid->Add(labelLabel, wxSizerFlags{}.CenterVertical());
        m_labelVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_labelVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_labelVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // track layout options
        auto* trackSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        trackSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Tracks:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString trackCountChoices;
        trackCountChoices.Add(_(L"Automatic"));
        trackCountChoices.Add(_(L"One"));
        trackCountChoices.Add(_(L"Two"));
        trackSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     trackCountChoices, 0,
                                     wxGenericValidator(&m_trackCountSelection)));

        trackSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Start angle (degrees):")),
                        wxSizerFlags{}.CenterVertical());
            {
            m_startAngleSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY);
            m_startAngleSpin->SetRange(0, 359.9);
            m_startAngleSpin->SetDigits(1);
            m_startAngleSpin->SetIncrement(5);
            m_startAngleSpin->SetValue(270.0);
            trackSizer->Add(m_startAngleSpin);
            }

        trackSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Track proportion:")),
                        wxSizerFlags{}.CenterVertical());
            {
            m_trackProportionSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY);
            m_trackProportionSpin->SetRange(0.3, 0.9);
            m_trackProportionSpin->SetDigits(2);
            m_trackProportionSpin->SetIncrement(0.05);
            m_trackProportionSpin->SetValue(0.65);
            trackSizer->Add(m_trackProportionSpin);
            }

        optionsSizer->Add(trackSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show track lane labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showLabels)),
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
    void InsertRaceTrackChartDlg::OnDatasetChanged()
        {
        m_valueVariable.clear();
        m_labelVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertRaceTrackChartDlg::OnSelectVariables()
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
                  .Label(_(L"Value"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_valueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_valueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Label"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_labelVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_labelVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto valueVars = dlg.GetSelectedVariables(0);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        const auto labelVars = dlg.GetSelectedVariables(1);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertRaceTrackChartDlg::UpdateVariableLabels()
        {
        m_valueVarLabel->SetLabel(m_valueVariable);
        m_labelVarLabel->SetLabel(m_labelVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertRaceTrackChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertRaceTrackChartDlg::GetSelectedDataset() const
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
    Graphs::RaceTrackChart::TrackCount InsertRaceTrackChartDlg::GetTrackCount() const noexcept
        {
        switch (m_trackCountSelection)
            {
        case 0:
            return Graphs::RaceTrackChart::TrackCount::Auto;
        case 2:
            return Graphs::RaceTrackChart::TrackCount::Two;
        default:
            return Graphs::RaceTrackChart::TrackCount::One;
            }
        }

    //-------------------------------------------
    double InsertRaceTrackChartDlg::GetTrackProportion() const
        {
        return (m_trackProportionSpin != nullptr) ? m_trackProportionSpin->GetValue() : 0.65;
        }

    //-------------------------------------------
    double InsertRaceTrackChartDlg::GetStartAngle() const
        {
        return (m_startAngleSpin != nullptr) ? m_startAngleSpin->GetValue() : 270.0;
        }

    //-------------------------------------------
    bool InsertRaceTrackChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_valueVariable.empty() || m_labelVariable.empty())
            {
            wxMessageBox(_(L"Please select the value and label variables."),
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
    void InsertRaceTrackChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* raceTrack = dynamic_cast<const Graphs::RaceTrackChart*>(&graph);
        if (raceTrack == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = raceTrack->GetPropertyTemplate(L"dataset");
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
        m_valueVariable = raceTrack->GetValueColumnName();
        m_labelVariable = raceTrack->GetLabelColumnName();
        UpdateVariableLabels();

        // track layout options
        switch (raceTrack->GetTrackCount())
            {
        case Graphs::RaceTrackChart::TrackCount::Auto:
            m_trackCountSelection = 0;
            break;
        case Graphs::RaceTrackChart::TrackCount::Two:
            m_trackCountSelection = 2;
            break;
        case Graphs::RaceTrackChart::TrackCount::One:
            m_trackCountSelection = 1;
            break;
            }
        m_showLabels = raceTrack->IsShowingLabels();

        m_startAngleSpin->SetValue(raceTrack->GetStartAngle());
        m_trackProportionSpin->SetValue(raceTrack->GetTrackProportion());

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
