///////////////////////////////////////////////////////////////////////////////
// Name:        insertwilmarthbridgeplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertwilmarthbridgeplotdlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <array>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWilmarthBridgePlotDlg::InsertWilmarthBridgePlotDlg(
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
    std::vector<DataGuideParameter> InsertWilmarthBridgePlotDlg::GetParameters()
        {
        const std::vector<Data::Dataset::ColumnImportType> labelTypes{
            Data::Dataset::ColumnImportType::String, Data::Dataset::ColumnImportType::Discrete,
            Data::Dataset::ColumnImportType::DichotomousString,
            Data::Dataset::ColumnImportType::DichotomousDiscrete,
            Data::Dataset::ColumnImportType::Numeric
        };
        const std::vector<Data::Dataset::ColumnImportType> periodTypes{
            Data::Dataset::ColumnImportType::Numeric, Data::Dataset::ColumnImportType::Date
        };
        const std::vector<Data::Dataset::ColumnImportType> flagTypes{
            Data::Dataset::ColumnImportType::Numeric, Data::Dataset::ColumnImportType::Discrete,
            Data::Dataset::ColumnImportType::DichotomousDiscrete
        };

        // the order here is the order of the lists in the variable selection dialog
        return {
            { .m_name = _(L"Label"),
              .m_types = labelTypes,
              .m_requirement = DataGuideRequirement::Required,
              .m_description =
                  _(L"Identifies each observation, which is drawn as one column of cells. Rows "
                    "sharing a label are combined into a single observation."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Entry"),
              .m_types = periodTypes,
              .m_requirement = DataGuideRequirement::Optional,
              .m_description =
                  _(L"The period the observation entered the study. Defaults to the first period. "
                    "Must be the same type as Exit."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Exit"),
              .m_types = periodTypes,
              .m_requirement = DataGuideRequirement::Required,
              .m_description =
                  _(L"The last period the observation was under observation, whether its event "
                    "occurred or follow-up simply ended."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Status"),
              .m_types = flagTypes,
              .m_requirement = DataGuideRequirement::Optional,
              .m_description =
                  _(L"Whether the exit was an event or a censoring. Missing counts as an event."),
              .m_values = { { L"1", _(L"Event occurred") }, { L"0", _(L"Censored") } },
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Intermediate Event"),
              .m_types = flagTypes,
              .m_requirement = DataGuideRequirement::Optional,
              .m_description =
                  _(L"Marks an event that happens partway through the study. Cells from that "
                    L"point on are drawn in a different color."),
              .m_values = { { L"0", _(L"Before the event") }, { L"1", _(L"After the event") } },
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} }
        };
        }

    //-------------------------------------------
    DataGuide InsertWilmarthBridgePlotDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Wilmarth Bridge Plot");
        guide.m_iconName = L"images/wilmarth-bridge.svg";
        guide.m_summary =
            _(L"Shows observations fading out of a study over time as a grid of characters, "
              "where the tapering silhouette traces the survival curve.");
        guide.m_requirements =
            _(L"One row per observation, with the period it entered the study and the period "
              "it exited.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample inscription;
        inscription.m_title = _(L"Example: fading inscription");
        inscription.m_columns = { { _(L"Letter"), _(L"Label") },
                                  { _(L"Entered"), _(L"Entry") },
                                  { _(L"Faded"), _(L"Exit") },
                                  { _(L"Status"), _(L"Status") } };
        inscription.m_rows = { { L"F", L"1963", L"1967", L"" },
                               { L"O", L"1963", L"1963", L"" },
                               { L"R", L"1963", L"1971", L"0" } };

        DataGuideExample patients;
        patients.m_title = _(L"Example: survival study with an intermediate event");
        patients.m_columns = { { _(L"Patient"), _(L"Label") },
                               { _(L"Start"), _(L"Entry") },
                               { _(L"Stop"), _(L"Exit") },
                               { _(L"Event"), _(L"Status") },
                               { _(L"Transplant"), _(L"Intermediate Event") } };
        patients.m_rows = {
            { L"P01", L"0", L"49", L"1", L"0" }, { L"P02", L"0", L"5", L"1", L"0" },
            { L"P03", L"0", L"12", L"0", L"0" }, { L"P03", L"12", L"41", L"1", L"1" },
            { L"P04", L"0", L"36", L"0", L"0" }, { L"P05", L"0", L"8", L"0", L"0" },
            { L"P05", L"8", L"27", L"0", L"1" }
        };
        guide.m_examples = { std::move(inscription), std::move(patients) };
        guide.m_missingData = {
            _(L"Label: a missing label is still drawn and counted, just with blank cells."),
            _(L"Entry: a missing value defaults to the first period."),
            _(L"Exit: a row with no exit value is left out of the statistics, but its column "
              "is still reserved in the plot. A fully empty row separates groups visually."),
            _(L"Status: a missing value is treated as an event, not a censoring.")
        };
        guide.m_notes = { _(L"The entry and exit columns can be dates instead of numbers, as "
                            "long as both are the same type.") };
        return guide;
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Wilmarth Bridge Plot"), ID_OPTIONS_SECTION,
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

        const auto addVarRow = [varGrid, varsBox](const wxString& caption)
        {
            auto* rowLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, caption);
            rowLabel->SetFont(rowLabel->GetFont().Bold());
            varGrid->Add(rowLabel, wxSizerFlags{}.CenterVertical());
            auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
            valueLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
            varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
            return valueLabel;
        };

        m_labelVarLabel = addVarRow(_(L"Label:"));
        m_entryVarLabel = addVarRow(_(L"Entry:"));
        m_exitVarLabel = addVarRow(_(L"Exit:"));
        m_statusVarLabel = addVarRow(_(L"Status:"));
        m_intermediateEventVarLabel = addVarRow(_(L"Intermediate event:"));

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // display options
        auto* displaySizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Fade effect:")),
                          wxSizerFlags{}.CenterVertical());
        wxArrayString fadeChoices;
        fadeChoices.Add(_(L"None"));
        fadeChoices.Add(_(L"Remaining lifetime"));
        fadeChoices.Add(_(L"Elapsed time"));
        displaySizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       fadeChoices, 0,
                                       wxGenericValidator{ &m_fadeEffectSelection }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Cell display:")),
                          wxSizerFlags{}.CenterVertical());
        wxArrayString anonymizeChoices;
        anonymizeChoices.Add(_(L"Labels"));
        anonymizeChoices.Add(_(L"Dots (anonymized)"));
        displaySizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       anonymizeChoices, 0,
                                       wxGenericValidator{ &m_anonymizeSelection }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Survival statistics:")),
                          wxSizerFlags{}.CenterVertical());
        wxArrayString survivalChoices;
        survivalChoices.Add(_(L"None"));
        survivalChoices.Add(_(L"At-risk count"));
        survivalChoices.Add(_(L"Survival percent"));
        survivalChoices.Add(_(L"Both"));
        displaySizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       survivalChoices, 0,
                                       wxGenericValidator{ &m_survivalDisplaySelection }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Terminal row label:")),
                          wxSizerFlags{}.CenterVertical());
        displaySizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, 0,
                                         wxGenericValidator{ &m_terminalRowLabel }));

        displaySizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Intermediate event color:")),
                          wxSizerFlags{}.CenterVertical());
        m_intermediateEventColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::Emerald));
        displaySizer->Add(m_intermediateEventColorPicker, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(displaySizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show censored markers"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showCensoredMarkers }),
                          wxSizerFlags{}.Border());

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage(false, 4);
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::OnDatasetChanged()
        {
        m_labelVariable.clear();
        m_exitVariable.clear();
        m_entryVariable.clear();
        m_statusVariable.clear();
        m_intermediateEventVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::OnSelectVariables()
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

        const std::array<wxString, 5> currentVariables{ m_labelVariable, m_entryVariable,
                                                        m_exitVariable, m_statusVariable,
                                                        m_intermediateEventVariable };
        const auto parameters = GetParameters();
        std::vector<VariableSelectDlg::VariableListInfo> variableLists;
        for (size_t paramIndex = 0; paramIndex < parameters.size(); ++paramIndex)
            {
            variableLists.push_back(MakeVariableList(
                parameters[paramIndex], currentVariables[paramIndex].empty() ?
                                            std::vector<wxString>{} :
                                            std::vector<wxString>{ currentVariables[paramIndex] }));
            }
        VariableSelectDlg dlg(this, columnInfo, variableLists);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto labelVars = dlg.GetSelectedVariables(0);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        const auto entryVars = dlg.GetSelectedVariables(1);
        m_entryVariable = entryVars.empty() ? wxString{} : entryVars.front();

        const auto exitVars = dlg.GetSelectedVariables(2);
        m_exitVariable = exitVars.empty() ? wxString{} : exitVars.front();

        const auto statusVars = dlg.GetSelectedVariables(3);
        m_statusVariable = statusVars.empty() ? wxString{} : statusVars.front();

        const auto intermediateEventVars = dlg.GetSelectedVariables(4);
        m_intermediateEventVariable =
            intermediateEventVars.empty() ? wxString{} : intermediateEventVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWilmarthBridgePlotDlg::UpdateVariableLabels()
        {
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_exitVarLabel->SetLabel(m_exitVariable);
        m_entryVarLabel->SetLabel(m_entryVariable);
        m_statusVarLabel->SetLabel(m_statusVariable);
        m_intermediateEventVarLabel->SetLabel(m_intermediateEventVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    wxColour InsertWilmarthBridgePlotDlg::GetIntermediateEventColor() const
        {
        return (m_intermediateEventColorPicker != nullptr) ?
                   m_intermediateEventColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::Emerald);
        }

    //-------------------------------------------
    Graphs::WilmarthBridgePlot::FadeEffect
    InsertWilmarthBridgePlotDlg::GetFadeEffect() const noexcept
        {
        switch (m_fadeEffectSelection)
            {
        case 1:
            return Graphs::WilmarthBridgePlot::FadeEffect::RemainingLifetime;
        case 2:
            return Graphs::WilmarthBridgePlot::FadeEffect::ElapsedTime;
        default:
            return Graphs::WilmarthBridgePlot::FadeEffect::None;
            }
        }

    //-------------------------------------------
    Graphs::WilmarthBridgePlot::Anonymize InsertWilmarthBridgePlotDlg::GetAnonymize() const noexcept
        {
        return (m_anonymizeSelection == 1) ? Graphs::WilmarthBridgePlot::Anonymize::Dots :
                                             Graphs::WilmarthBridgePlot::Anonymize::None;
        }

    //-------------------------------------------
    Graphs::WilmarthBridgePlot::SurvivalDisplay
    InsertWilmarthBridgePlotDlg::GetSurvivalDisplay() const noexcept
        {
        switch (m_survivalDisplaySelection)
            {
        case 1:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::AtRiskCount;
        case 2:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::SurvivalPercent;
        case 3:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::Both;
        default:
            return Graphs::WilmarthBridgePlot::SurvivalDisplay::None;
            }
        }

    //-------------------------------------------
    bool InsertWilmarthBridgePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_labelVariable.empty() || m_exitVariable.empty())
            {
            wxMessageBox(_(L"Please select the label and exit variables."),
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
    void InsertWilmarthBridgePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* bridgePlot = dynamic_cast<const Graphs::WilmarthBridgePlot*>(&graph);
        if (bridgePlot == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(bridgePlot->GetPropertyTemplate(L"dataset"));

        // load column names from the graph
        m_labelVariable = bridgePlot->GetLabelColumnName();
        m_exitVariable = bridgePlot->GetExitColumnName();
        m_entryVariable = bridgePlot->GetEntryColumnName();
        m_statusVariable = bridgePlot->GetStatusColumnName();
        m_intermediateEventVariable = bridgePlot->GetIntermediateEventColumnName();
        UpdateVariableLabels();

        if (m_intermediateEventColorPicker != nullptr)
            {
            m_intermediateEventColorPicker->SetColour(bridgePlot->GetIntermediateEventColor());
            }

        // display options
        switch (bridgePlot->GetFadeEffect())
            {
        case Graphs::WilmarthBridgePlot::FadeEffect::RemainingLifetime:
            m_fadeEffectSelection = 1;
            break;
        case Graphs::WilmarthBridgePlot::FadeEffect::ElapsedTime:
            m_fadeEffectSelection = 2;
            break;
        case Graphs::WilmarthBridgePlot::FadeEffect::None:
            m_fadeEffectSelection = 0;
            break;
            }

        m_anonymizeSelection =
            (bridgePlot->GetAnonymize() == Graphs::WilmarthBridgePlot::Anonymize::Dots) ? 1 : 0;

        switch (bridgePlot->GetSurvivalDisplay())
            {
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::AtRiskCount:
            m_survivalDisplaySelection = 1;
            break;
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::SurvivalPercent:
            m_survivalDisplaySelection = 2;
            break;
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::Both:
            m_survivalDisplaySelection = 3;
            break;
        case Graphs::WilmarthBridgePlot::SurvivalDisplay::None:
            m_survivalDisplaySelection = 0;
            break;
            }

        m_showCensoredMarkers = bridgePlot->IsShowingCensoredMarkers();

        const auto terminalRowTmpl = bridgePlot->GetPropertyTemplate(L"terminal-row-label");
        m_terminalRowLabel =
            terminalRowTmpl.empty() ? bridgePlot->GetTerminalRowLabel() : terminalRowTmpl;

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::WilmarthBridgePlot>
    InsertWilmarthBridgePlotDlg::BuildWilmarthBridgePlot(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::WilmarthBridgePlot>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetData(GetSelectedDataset(), GetLabelVariable(), GetExitVariable(),
                      (GetEntryVariable().empty() ? std::nullopt :
                                                    std::optional<wxString>(GetEntryVariable())),
                      (GetStatusVariable().empty() ? std::nullopt :
                                                     std::optional<wxString>(GetStatusVariable())),
                      (GetIntermediateEventVariable().empty() ?
                           std::nullopt :
                           std::optional<wxString>(GetIntermediateEventVariable())));

        plot->SetFadeEffect(GetFadeEffect());
        plot->SetAnonymize(GetAnonymize());
        plot->SetSurvivalDisplay(GetSurvivalDisplay());
        plot->ShowCensoredMarkers(IsShowingCensoredMarkers());
        plot->ShowTerminalRow(GetTerminalRowLabel());
        plot->SetIntermediateEventColor(GetIntermediateEventColor());

        if (oldGraph != nullptr)
            {
            ApplyAxisOverrides(*plot);

            const auto* oldPlot = dynamic_cast<const Graphs::WilmarthBridgePlot*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.label", GetLabelVariable(),
                oldPlot != nullptr ? oldPlot->GetLabelColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.exit", GetExitVariable(),
                oldPlot != nullptr ? oldPlot->GetExitColumnName() : wxString{});
            if (!GetEntryVariable().empty())
                {
                WisteriaView::CarryForwardProperty(
                    *oldGraph, *plot, L"variables.entered", GetEntryVariable(),
                    oldPlot != nullptr ? oldPlot->GetEntryColumnName() : wxString{});
                }
            if (!GetStatusVariable().empty())
                {
                WisteriaView::CarryForwardProperty(
                    *oldGraph, *plot, L"variables.status", GetStatusVariable(),
                    oldPlot != nullptr ? oldPlot->GetStatusColumnName() : wxString{});
                }
            if (!GetIntermediateEventVariable().empty())
                {
                WisteriaView::CarryForwardProperty(
                    *oldGraph, *plot, L"variables.intermediate-event",
                    GetIntermediateEventVariable(),
                    oldPlot != nullptr ? oldPlot->GetIntermediateEventColumnName() : wxString{});
                }
            if (!GetTerminalRowLabel().empty())
                {
                WisteriaView::CarryForwardProperty(
                    *oldGraph, *plot, L"terminal-row-label", GetTerminalRowLabel(),
                    oldPlot != nullptr ? oldPlot->GetTerminalRowLabel() : wxString{});
                }
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.label", GetLabelVariable());
            plot->SetPropertyTemplate(L"variables.exit", GetExitVariable());
            if (!GetEntryVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.entered", GetEntryVariable());
                }
            if (!GetStatusVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.status", GetStatusVariable());
                }
            if (!GetIntermediateEventVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.intermediate-event",
                                          GetIntermediateEventVariable());
                }
            if (!GetTerminalRowLabel().empty())
                {
                plot->SetPropertyTemplate(L"terminal-row-label", GetTerminalRowLabel());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
