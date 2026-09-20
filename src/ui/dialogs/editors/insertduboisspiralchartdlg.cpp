///////////////////////////////////////////////////////////////////////////////
// Name:        insertduboisspiralchartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertduboisspiralchartdlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertDuBoisSpiralChartDlg::InsertDuBoisSpiralChartDlg(
        Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
        const wxString& caption, const wxWindowID id, const wxPoint& pos, const wxSize& size,
        const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertDuBoisSpiralChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Du Bois Spiral Chart"), ID_OPTIONS_SECTION,
                                  true);

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

        auto* labelLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Label:"));
        labelLabel->SetFont(labelLabel->GetFont().Bold());
        varGrid->Add(labelLabel, wxSizerFlags{}.CenterVertical());
        m_labelVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_labelVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_labelVarLabel, wxSizerFlags{}.CenterVertical());

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Value:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // layout and display options
        auto* layoutSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Value display:")),
                         wxSizerFlags{}.CenterVertical());
        wxArrayString valueFormatChoices;
        valueFormatChoices.Add(_(L"Value"));
        valueFormatChoices.Add(_(L"Percentage"));
        valueFormatChoices.Add(_(L"Currency"));
        valueFormatChoices.Add(_(L"Value (no thousands separators)"));
        layoutSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      valueFormatChoices, 0,
                                      wxGenericValidator{ &m_valueFormatIndex }));

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Zigzag angle (degrees):")),
                         wxSizerFlags{}.CenterVertical());
            {
            auto* angleSpin = new wxSpinCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                             wxSize{ FromDIP(80), -1 });
            angleSpin->SetRange(10, 80);
            angleSpin->SetValue(m_zigZagAngle);
            angleSpin->SetValidator(wxGenericValidator{ &m_zigZagAngle });
            layoutSizer->Add(angleSpin);
            }

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Spiral radius (%):")),
                         wxSizerFlags{}.CenterVertical());
        m_outerRadiusSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY, wxString{},
                                                 wxDefaultPosition, wxSize{ FromDIP(80), -1 });
        m_outerRadiusSpin->SetRange(15, 48);
        m_outerRadiusSpin->SetDigits(0);
        m_outerRadiusSpin->SetIncrement(1);
        m_outerRadiusSpin->SetValue(Graphs::DuBoisSpiralChart::DEFAULT_OUTER_RADIUS_PROPORTION *
                                    100);
        layoutSizer->Add(m_outerRadiusSpin);

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Line thickness (%):")),
                         wxSizerFlags{}.CenterVertical());
        m_lineThicknessSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY, wxString{},
                                                   wxDefaultPosition, wxSize{ FromDIP(80), -1 });
        m_lineThicknessSpin->SetRange(0.5, 8);
        m_lineThicknessSpin->SetDigits(1);
        m_lineThicknessSpin->SetIncrement(0.5);
        m_lineThicknessSpin->SetValue(Graphs::DuBoisSpiralChart::DEFAULT_LINE_THICKNESS_PROPORTION *
                                      100);
        layoutSizer->Add(m_lineThicknessSpin);

        optionsSizer->Add(layoutSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showLabels }),
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
    NumberDisplay InsertDuBoisSpiralChartDlg::GetValueFormat() const noexcept
        {
        switch (m_valueFormatIndex)
            {
        case 1:
            return NumberDisplay::Percentage;
        case 2:
            return NumberDisplay::Currency;
        case 3:
            return NumberDisplay::ValueSimple;
        default:
            return NumberDisplay::Value;
            }
        }

    //-------------------------------------------
    double InsertDuBoisSpiralChartDlg::GetOuterRadiusProportion() const
        {
        return (m_outerRadiusSpin != nullptr) ?
                   m_outerRadiusSpin->GetValue() / 100 :
                   Graphs::DuBoisSpiralChart::DEFAULT_OUTER_RADIUS_PROPORTION;
        }

    //-------------------------------------------
    double InsertDuBoisSpiralChartDlg::GetLineThicknessProportion() const
        {
        return (m_lineThicknessSpin != nullptr) ?
                   m_lineThicknessSpin->GetValue() / 100 :
                   Graphs::DuBoisSpiralChart::DEFAULT_LINE_THICKNESS_PROPORTION;
        }

    //-------------------------------------------
    void InsertDuBoisSpiralChartDlg::OnDatasetChanged()
        {
        m_labelVariable.clear();
        m_valueVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertDuBoisSpiralChartDlg::OnSelectVariables()
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
                  .Label(_(L"Label"))
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
                  .Label(_(L"Value"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_valueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_valueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto labelVars = dlg.GetSelectedVariables(0);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        const auto valueVars = dlg.GetSelectedVariables(1);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertDuBoisSpiralChartDlg::UpdateVariableLabels()
        {
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_valueVarLabel->SetLabel(m_valueVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertDuBoisSpiralChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertDuBoisSpiralChartDlg::GetSelectedDataset() const
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
    bool InsertDuBoisSpiralChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_labelVariable.empty() || m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the label and value variables."),
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
    void InsertDuBoisSpiralChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* spiralChart = dynamic_cast<const Graphs::DuBoisSpiralChart*>(&graph);
        if (spiralChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = spiralChart->GetPropertyTemplate(L"dataset");
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
        m_labelVariable = spiralChart->GetLabelColumnName();
        m_valueVariable = spiralChart->GetValueColumnName();
        UpdateVariableLabels();

        // layout and display options
        switch (spiralChart->GetValueFormat())
            {
        case NumberDisplay::Percentage:
            m_valueFormatIndex = 1;
            break;
        case NumberDisplay::Currency:
            m_valueFormatIndex = 2;
            break;
        case NumberDisplay::ValueSimple:
            m_valueFormatIndex = 3;
            break;
        default:
            m_valueFormatIndex = 0;
            break;
            }
        m_showLabels = spiralChart->IsShowingLabels();
        m_zigZagAngle = static_cast<int>(std::lround(spiralChart->GetZigZagAngle()));
        m_outerRadiusSpin->SetValue(spiralChart->GetOuterRadiusProportion() * 100);
        m_lineThicknessSpin->SetValue(spiralChart->GetLineThicknessProportion() * 100);

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::DuBoisSpiralChart>
    InsertDuBoisSpiralChartDlg::BuildDuBoisSpiralChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::DuBoisSpiralChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        // sets the color scheme, which SetData() uses to color the segments
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetValueFormat(GetValueFormat());
        plot->ShowLabels(IsShowingLabels());
        plot->SetZigZagAngle(GetZigZagAngle());
        plot->SetOuterRadiusProportion(GetOuterRadiusProportion());
        plot->SetLineThicknessProportion(GetLineThicknessProportion());

        plot->SetData(GetSelectedDataset(), GetValueVariable(), GetLabelVariable());

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::DuBoisSpiralChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.label", GetLabelVariable(),
                oldChart != nullptr ? oldChart->GetLabelColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldChart != nullptr ? oldChart->GetValueColumnName() : wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.label", GetLabelVariable());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
