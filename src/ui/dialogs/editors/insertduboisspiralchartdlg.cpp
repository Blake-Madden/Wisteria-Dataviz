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
    std::vector<DataGuideParameter> InsertDuBoisSpiralChartDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"Label"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The name of each segment of the spiral."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Value"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The amount for each segment. The length of the segment is "
                                      "proportional to it."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertDuBoisSpiralChartDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Du Bois Spiral Chart");
        guide.m_iconName = L"images/dubois-spiral.svg";
        guide.m_summary =
            _(L"Draws values as segments of one long path that zigzags down and then winds "
              "into a spiral, which suits data where the last value is far larger than the "
              "others.");
        guide.m_requirements =
            _(L"One row per segment, in the order that the segments should be drawn, holding "
              "its name and its value.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample population;
        population.m_title = _(L"Example: population by size of place");
        population.m_columns = { { _(L"Place"), _(L"Label") }, { _(L"Population"), _(L"Value") } };
        population.m_rows = { { _(L"Under 2,500"), L"500" },
                              { _(L"2,500 to 5,000"), L"700" },
                              { _(L"5,000 to 10,000"), L"900" },
                              { _(L"Over 10,000"), L"4000" } };
        guide.m_examples = { std::move(population) };

        guide.m_notes = { _(L"Row order matters. The first row is drawn across the top and "
                            "the last row is drawn as the innermost part of the spiral.") };
        return guide;
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
        auto* datasetChoice = CreateDatasetChoice(optionsPage);
        datasetSizer->Add(datasetChoice);

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
        datasetChoice->Bind(wxEVT_CHOICE,
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
            const auto foundPos = importOpts.find(GetSelectedDatasetName());
            if (foundPos != importOpts.cend())
                {
                columnInfo = foundPos->second.m_columnPreviewInfo;
                }
            }
        if (columnInfo.empty())
            {
            columnInfo = BuildColumnPreviewInfo(*dataset);
            }

        const auto parameters = GetParameters();
        VariableSelectDlg dlg(
            this, columnInfo,
            { MakeVariableList(parameters[0], m_labelVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_labelVariable }),
              MakeVariableList(parameters[1], m_valueVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_valueVariable }) });

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
        SelectDataset(spiralChart->GetPropertyTemplate(L"dataset"));

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
