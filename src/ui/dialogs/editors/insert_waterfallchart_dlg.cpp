///////////////////////////////////////////////////////////////////////////////
// Name:        insert_waterfallchart_dlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insert_waterfallchart_dlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWaterfallChartDlg::InsertWaterfallChartDlg(Canvas* canvas,
                                                     const ReportBuilder* reportBuilder,
                                                     wxWindow* parent, const wxString& caption,
                                                     const wxWindowID id, const wxPoint& pos,
                                                     const wxSize& size, const long style,
                                                     EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeNone)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertWaterfallChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Waterfall Chart"), ID_OPTIONS_SECTION, true);

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

        auto* labelLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Step label:"));
        labelLabel->SetFont(labelLabel->GetFont().Bold());
        varGrid->Add(labelLabel, wxSizerFlags{}.CenterVertical());
        m_labelVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_labelVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_labelVarLabel, wxSizerFlags{}.CenterVertical());

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Amount:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        auto* totalFlagLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Total flag (optional):"));
        totalFlagLabel->SetFont(totalFlagLabel->GetFont().Bold());
        varGrid->Add(totalFlagLabel, wxSizerFlags{}.CenterVertical());
        m_totalFlagVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_totalFlagVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_totalFlagVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // bar orientation and value display options
        auto* valueSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        valueSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Orientation:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString orientationChoices;
        orientationChoices.Add(_(L"Vertical"));
        orientationChoices.Add(_(L"Horizontal"));
        valueSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     orientationChoices, 0,
                                     wxGenericValidator{ &m_orientationIndex }));
        valueSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Value format:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString valueFormatChoices;
        valueFormatChoices.Add(_(L"Value"));
        valueFormatChoices.Add(_(L"Currency"));
        valueFormatChoices.Add(_(L"Percentage"));
        valueFormatChoices.Add(_(L"Simple value"));
        valueSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     valueFormatChoices, 0,
                                     wxGenericValidator{ &m_valueFormatIndex }));
        optionsSizer->Add(valueSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show bar values"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showBarValues }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show values on blocks"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showBlockValues }),
                          wxSizerFlags{}.Border());

        // bar colors
        auto* colorBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Bar Colors"));

        auto* colorGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        colorGrid->Add(new wxStaticText(colorBox->GetStaticBox(), wxID_ANY, _(L"Increase color:")),
                       wxSizerFlags{}.CenterVertical());
        m_increaseColorPicker =
            new wxColourPickerCtrl(colorBox->GetStaticBox(), wxID_ANY,
                                   Colors::ColorBrewer::GetColor(Colors::Color::Emerald));
        colorGrid->Add(m_increaseColorPicker);

        colorGrid->Add(new wxStaticText(colorBox->GetStaticBox(), wxID_ANY, _(L"Decrease color:")),
                       wxSizerFlags{}.CenterVertical());
        m_decreaseColorPicker =
            new wxColourPickerCtrl(colorBox->GetStaticBox(), wxID_ANY,
                                   Colors::ColorBrewer::GetColor(Colors::Color::Tangerine));
        colorGrid->Add(m_decreaseColorPicker);

        colorGrid->Add(new wxStaticText(colorBox->GetStaticBox(), wxID_ANY, _(L"Total color:")),
                       wxSizerFlags{}.CenterVertical());
        m_totalColorPicker =
            new wxColourPickerCtrl(colorBox->GetStaticBox(), wxID_ANY,
                                   Colors::ColorBrewer::GetColor(Colors::Color::BabyBlue));
        colorGrid->Add(m_totalColorPicker);

        colorBox->Add(colorGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(colorBox, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertWaterfallChartDlg::OnDatasetChanged()
        {
        m_labelVariable.clear();
        m_valueVariable.clear();
        m_totalFlagVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWaterfallChartDlg::OnSelectVariables()
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
                  .Label(_(L"Step label"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_labelVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_labelVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Amount"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_valueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_valueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Total flag (0 = change, 1 = total)"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_totalFlagVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_totalFlagVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete,
                                   Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::DichotomousString }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto labelVars = dlg.GetSelectedVariables(0);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        const auto valueVars = dlg.GetSelectedVariables(1);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        const auto totalFlagVars = dlg.GetSelectedVariables(2);
        m_totalFlagVariable = totalFlagVars.empty() ? wxString{} : totalFlagVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWaterfallChartDlg::UpdateVariableLabels()
        {
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_valueVarLabel->SetLabel(m_valueVariable);
        m_totalFlagVarLabel->SetLabel(m_totalFlagVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertWaterfallChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertWaterfallChartDlg::GetSelectedDataset() const
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
    wxColour InsertWaterfallChartDlg::GetIncreaseColor() const
        {
        return (m_increaseColorPicker != nullptr) ?
                   m_increaseColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::Emerald);
        }

    //-------------------------------------------
    wxColour InsertWaterfallChartDlg::GetDecreaseColor() const
        {
        return (m_decreaseColorPicker != nullptr) ?
                   m_decreaseColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::Tangerine);
        }

    //-------------------------------------------
    wxColour InsertWaterfallChartDlg::GetTotalColor() const
        {
        return (m_totalColorPicker != nullptr) ?
                   m_totalColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::BabyBlue);
        }

    //-------------------------------------------
    bool InsertWaterfallChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_labelVariable.empty() || m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the step label and amount variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertWaterfallChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* waterfallChart = dynamic_cast<const Graphs::WaterfallChart*>(&graph);
        if (waterfallChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = waterfallChart->GetPropertyTemplate(L"dataset");
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
        m_labelVariable = waterfallChart->GetLabelColumnName();
        m_valueVariable = waterfallChart->GetValueColumnName();
        m_totalFlagVariable = waterfallChart->GetTotalFlagColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // orientation and value display options
        m_orientationIndex =
            (waterfallChart->GetBarOrientation() == Wisteria::Orientation::Horizontal) ? 1 : 0;
        switch (waterfallChart->GetValueDisplay())
            {
        case NumberDisplay::Currency:
            m_valueFormatIndex = 1;
            break;
        case NumberDisplay::Percentage:
            m_valueFormatIndex = 2;
            break;
        case NumberDisplay::ValueSimple:
            m_valueFormatIndex = 3;
            break;
        case NumberDisplay::Value:
            [[fallthrough]];
        default:
            m_valueFormatIndex = 0;
            }
        m_showBarValues = waterfallChart->IsShowingBarValues();
        m_showBlockValues = waterfallChart->IsShowingBlockValues();

        // bar colors
        if (m_increaseColorPicker != nullptr)
            {
            m_increaseColorPicker->SetColour(waterfallChart->GetIncreaseColor());
            }
        if (m_decreaseColorPicker != nullptr)
            {
            m_decreaseColorPicker->SetColour(waterfallChart->GetDecreaseColor());
            }
        if (m_totalColorPicker != nullptr)
            {
            m_totalColorPicker->SetColour(waterfallChart->GetTotalColor());
            }

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::WaterfallChart>
    InsertWaterfallChartDlg::BuildWaterfallChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::WaterfallChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetBarOrientation(GetOrientation());
        plot->SetIncreaseColor(GetIncreaseColor());
        plot->SetDecreaseColor(GetDecreaseColor());
        plot->SetTotalColor(GetTotalColor());
        plot->SetValueDisplay(GetValueDisplay());
        plot->ShowBarValues(IsShowingBarValues());
        plot->ShowBlockValues(IsShowingBlockValues());

        plot->SetData(GetSelectedDataset(), GetLabelVariable(), GetValueVariable(),
                      GetTotalFlagVariable().empty() ?
                          std::nullopt :
                          std::optional<wxString>{ GetTotalFlagVariable() });

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::WaterfallChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.label", GetLabelVariable(),
                oldChart != nullptr ? oldChart->GetLabelColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldChart != nullptr ? oldChart->GetValueColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.total-flag", GetTotalFlagVariable(),
                oldChart != nullptr ? oldChart->GetTotalFlagColumnName().value_or(wxString{}) :
                                      wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.label", GetLabelVariable());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            plot->SetPropertyTemplate(L"variables.total-flag", GetTotalFlagVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
