///////////////////////////////////////////////////////////////////////////////
// Name:        insertfunnelchartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertfunnelchartdlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertFunnelChartDlg::InsertFunnelChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    std::vector<DataGuideParameter> InsertFunnelChartDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"Stage"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description =
                       _(L"The name of each stage in the funnel, in the order that the "
                         "stages should appear."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Value"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The actual value at each stage, such as the number of "
                                      "people who reached it."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Target"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Optional,
                   .m_description = _(L"The value that each stage was expected to reach. It is "
                                      "drawn as a ghost behind the actual value."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertFunnelChartDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Funnel Chart");
        guide.m_iconName = L"images/funnel.svg";
        guide.m_summary =
            _(L"Shows how a quantity narrows through a series of stages, such as applicants "
              "moving through an admissions process, optionally against a target.");
        guide.m_requirements =
            _(L"One row per stage, in order, holding the stage's name, its actual value, and "
              "optionally its target value.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample admissions;
        admissions.m_title = _(L"Example: applicants moving through admissions");
        admissions.m_columns = { { _(L"Stage"), _(L"Stage") },
                                 { _(L"Count"), _(L"Value") },
                                 { _(L"Goal"), _(L"Target") } };
        admissions.m_rows = { { _(L"Applied"), L"1000", L"1100" },
                              { _(L"Accepted"), L"640", L"700" },
                              { _(L"Deposited"), L"310", L"350" },
                              { _(L"Enrolled"), L"280", L"300" } };
        guide.m_examples = { std::move(admissions) };

        guide.m_missingData = {
            _(L"A missing stage name is shown with an empty axis label, keeping its position."),
            _(L"A missing value is treated as 0, so the stage collapses to a line."),
            _(L"A missing target, or no target column, means no ghost is drawn for that stage.")
        };
        return guide;
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Funnel Chart"), ID_OPTIONS_SECTION, true);

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

        auto* stageLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Stage:"));
        stageLabel->SetFont(stageLabel->GetFont().Bold());
        varGrid->Add(stageLabel, wxSizerFlags{}.CenterVertical());
        m_stageVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_stageVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_stageVarLabel, wxSizerFlags{}.CenterVertical());

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Value:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        auto* targetLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Target (optional):"));
        targetLabel->SetFont(targetLabel->GetFont().Bold());
        varGrid->Add(targetLabel, wxSizerFlags{}.CenterVertical());
        m_targetVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_targetVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_targetVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // style and display options
        auto* styleSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        styleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Funnel style:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString funnelStyleChoices;
        funnelStyleChoices.Add(_(L"Glassy"));
        funnelStyleChoices.Add(_(L"Standard"));
        styleSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     funnelStyleChoices, 0,
                                     wxGenericValidator{ &m_funnelStyleIndex }));
        styleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Target ghost opacity:")),
                        wxSizerFlags{}.CenterVertical());
            {
            auto* opacitySpin = new wxSpinCtrl(optionsPage, wxID_ANY);
            opacitySpin->SetRange(0, 255);
            opacitySpin->SetValue(m_targetGhostOpacity);
            opacitySpin->SetValidator(wxGenericValidator{ &m_targetGhostOpacity });
            styleSizer->Add(opacitySpin);
            }
        optionsSizer->Add(styleSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show explanations"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showExplanations }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show conversion labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showConversionLabels }),
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
    void InsertFunnelChartDlg::OnDatasetChanged()
        {
        m_stageVariable.clear();
        m_valueVariable.clear();
        m_targetVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::OnSelectVariables()
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
            { MakeVariableList(parameters[0], m_stageVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_stageVariable }),
              MakeVariableList(parameters[1], m_valueVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_valueVariable }),
              MakeVariableList(parameters[2], m_targetVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_targetVariable }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto stageVars = dlg.GetSelectedVariables(0);
        m_stageVariable = stageVars.empty() ? wxString{} : stageVars.front();

        const auto valueVars = dlg.GetSelectedVariables(1);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        const auto targetVars = dlg.GetSelectedVariables(2);
        m_targetVariable = targetVars.empty() ? wxString{} : targetVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertFunnelChartDlg::UpdateVariableLabels()
        {
        m_stageVarLabel->SetLabel(m_stageVariable);
        m_valueVarLabel->SetLabel(m_valueVariable);
        m_targetVarLabel->SetLabel(m_targetVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertFunnelChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_stageVariable.empty() || m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the stage and value variables."),
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
    void InsertFunnelChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* funnelChart = dynamic_cast<const Graphs::FunnelChart*>(&graph);
        if (funnelChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(funnelChart->GetPropertyTemplate(L"dataset"));

        // load column names from the graph
        m_stageVariable = funnelChart->GetStageColumnName();
        m_valueVariable = funnelChart->GetValueColumnName();
        m_targetVariable = funnelChart->GetTargetColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // style and display options
        m_funnelStyleIndex =
            (funnelChart->GetFunnelStyle() == Graphs::FunnelChart::FunnelStyle::Standard) ? 1 : 0;
        m_showExplanations = funnelChart->AreExplanationsShown();
        m_showConversionLabels = funnelChart->AreConversionLabelsShown();
        m_targetGhostOpacity = funnelChart->GetTargetGhostOpacity();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::FunnelChart>
    InsertFunnelChartDlg::BuildFunnelChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::FunnelChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetFunnelStyle(GetFunnelStyle());
        plot->ShowExplanations(IsShowingExplanations());
        plot->ShowConversionLabels(IsShowingConversionLabels());
        plot->SetTargetGhostOpacity(GetTargetGhostOpacity());

        plot->SetData(GetSelectedDataset(), GetStageVariable(), GetValueVariable(),
                      GetTargetVariable().empty() ? std::nullopt :
                                                    std::optional<wxString>{ GetTargetVariable() });

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::FunnelChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.stage", GetStageVariable(),
                oldChart != nullptr ? oldChart->GetStageColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldChart != nullptr ? oldChart->GetValueColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.target", GetTargetVariable(),
                oldChart != nullptr ? oldChart->GetTargetColumnName().value_or(wxString{}) :
                                      wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.stage", GetStageVariable());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            plot->SetPropertyTemplate(L"variables.target", GetTargetVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
