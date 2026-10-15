///////////////////////////////////////////////////////////////////////////////
// Name:        insertheatmapdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertheatmapdlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertHeatMapDlg::InsertHeatMapDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                       wxWindow* parent, const wxString& caption,
                                       const wxWindowID id, const wxPoint& pos, const wxSize& size,
                                       const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    std::vector<DataGuideParameter> InsertHeatMapDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"Continuous"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The measurement that is color mapped. Each row becomes one "
                                      "cell, colored by its value."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Group"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Optional,
                   .m_description = _(L"Arranges the cells into a separate row for each unique "
                                      "value, such as one row per student."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertHeatMapDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Heat Map");
        guide.m_iconName = L"images/heatmap.svg";
        guide.m_summary =
            _(L"Shows a measurement as a grid of colored cells, so that high and low values "
              "stand out, either as one block of cells or as one row per group.");
        guide.m_requirements =
            _(L"One row per cell, holding the measurement and, optionally, the group whose "
              "row the cell belongs to.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample scores;
        scores.m_title = _(L"Example: weekly test scores for two students");
        scores.m_columns = { { _(L"Name"), _(L"Group") },
                             { _(L"Score"), _(L"Continuous") },
                             { _(L"Week"), wxString{} } };
        scores.m_rows = { { _(L"Anna"), L"87", _(L"week 1") }, { _(L"Anna"), L"89", _(L"week 2") },
                          { _(L"Anna"), L"91", _(L"week 3") }, { _(L"Anna"), L"95", _(L"week 4") },
                          { _(L"Joe"), L"37", _(L"week 1") },  { _(L"Joe"), L"", _(L"week 2") },
                          { _(L"Joe"), L"", _(L"week 3") },    { _(L"Joe"), L"82", _(L"week 4") } };
        guide.m_examples = { std::move(scores) };

        guide.m_missingData = {
            _(L"A missing measurement is shown as a transparent cell with a red X in the "
              "middle."),
            _(L"A missing group is shown as a row with an empty label.")
        };
        return guide;
        }

    //-------------------------------------------
    void InsertHeatMapDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Heat Map"), ID_OPTIONS_SECTION, true);

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

        auto* contLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Continuous:"));
        contLabel->SetFont(contLabel->GetFont().Bold());
        varGrid->Add(contLabel, wxSizerFlags{}.CenterVertical());
        m_continuousVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_continuousVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_continuousVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Group:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // group column count
        auto* groupOptsSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        m_groupColumnCountLabel =
            new wxStaticText(optionsPage, wxID_ANY, _(L"Group column count:"));
        groupOptsSizer->Add(m_groupColumnCountLabel, wxSizerFlags{}.CenterVertical());
        m_groupColumnCountSpin = new wxSpinCtrl(optionsPage, wxID_ANY);
        m_groupColumnCountSpin->SetRange(1, 5);
        m_groupColumnCountSpin->SetValue(m_groupColumnCount);
        m_groupColumnCountSpin->SetValidator(wxGenericValidator{ &m_groupColumnCount });
        groupOptsSizer->Add(m_groupColumnCountSpin, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(groupOptsSizer, wxSizerFlags{}.Border());

        // show group headers
        m_showGroupHeadersCheck =
            new wxCheckBox(optionsPage, wxID_ANY, _(L"Show group headers"), wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_showGroupHeaders });
        optionsSizer->Add(m_showGroupHeadersCheck, wxSizerFlags{}.Border(wxLEFT));

        // group header prefix
        auto* prefixSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        m_groupHeaderPrefixLabel =
            new wxStaticText(optionsPage, wxID_ANY, _(L"Group header prefix:"));
        prefixSizer->Add(m_groupHeaderPrefixLabel, wxSizerFlags{}.CenterVertical());
        m_groupHeaderPrefixText =
            new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize, 0,
                           wxGenericValidator{ &m_groupHeaderPrefix });
        prefixSizer->Add(m_groupHeaderPrefixText, wxSizerFlags{}.CenterVertical().Expand());

        optionsSizer->Add(prefixSizer, wxSizerFlags{}.Border());

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        m_showGroupHeadersCheck->Bind(wxEVT_CHECKBOX, [this]([[maybe_unused]] wxCommandEvent&)
                                      { UpdateGroupControlStates(); });

        UpdateGroupControlStates();

        CreateLegendOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::OnDatasetChanged()
        {
        m_continuousVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::OnSelectVariables()
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
            const auto it = importOpts.find(GetSelectedDatasetName());
            if (it != importOpts.cend())
                {
                columnInfo = it->second.m_columnPreviewInfo;
                }
            }
        if (columnInfo.empty())
            {
            columnInfo = BuildColumnPreviewInfo(*dataset);
            }

        // expand any constant placeholders so the variable selection list
        // can match the real dataset columns
        const auto continuousDefault = ExpandVariable(m_continuousVariable);
        const auto groupDefault = ExpandVariable(m_groupVariable);

        const auto parameters = GetParameters();
        VariableSelectDlg dlg(
            this, columnInfo,
            { MakeVariableList(parameters[0], continuousDefault.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ continuousDefault }),
              MakeVariableList(parameters[1], groupDefault.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ groupDefault }) });

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
    void InsertHeatMapDlg::UpdateVariableLabels()
        {
        m_continuousVarLabel->SetLabel(m_continuousVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        UpdateGroupControlStates();

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertHeatMapDlg::UpdateGroupControlStates()
        {
        const bool hasGroup = !m_groupVariable.empty();
        const bool showHeaders = hasGroup && m_showGroupHeadersCheck->IsChecked();

        m_groupColumnCountLabel->Enable(hasGroup);
        m_groupColumnCountSpin->Enable(hasGroup);
        m_showGroupHeadersCheck->Enable(hasGroup);
        m_groupHeaderPrefixLabel->Enable(showHeaders);
        m_groupHeaderPrefixText->Enable(showHeaders);
        }

    //-------------------------------------------
    bool InsertHeatMapDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_continuousVariable.empty())
            {
            wxMessageBox(_(L"Please select a continuous variable."), _(L"Variable Not Specified"),
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
    void InsertHeatMapDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* heatmap = dynamic_cast<const Graphs::HeatMap*>(&graph);
        if (heatmap == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(heatmap->GetPropertyTemplate(L"dataset"));

        // load column names from property templates
        m_continuousVariable = heatmap->GetPropertyTemplate(L"variables.continuous");
        m_groupVariable = heatmap->GetPropertyTemplate(L"variables.group");
        UpdateVariableLabels();

        // group options
        if (heatmap->GetGroupColumnCount().has_value())
            {
            m_groupColumnCount = static_cast<int>(heatmap->GetGroupColumnCount().value());
            }
        m_showGroupHeaders = heatmap->IsShowingGroupHeaders();
        m_groupHeaderPrefix = heatmap->GetGroupHeaderPrefix();

        TransferDataToWindow();

        UpdateGroupControlStates();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::HeatMap> InsertHeatMapDlg::BuildHeatMap(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::HeatMap>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            GetGroupVariable().empty() ? std::nullopt : std::optional<wxString>(GetGroupVariable());
        plot->SetData(GetSelectedDataset(), GetContinuousVariable(), groupCol,
                      static_cast<size_t>(GetGroupColumnCount()));
        ApplyAxisOverrides(*plot);

        plot->ShowGroupHeaders(GetShowGroupHeaders());
        plot->SetGroupHeaderPrefix(GetGroupHeaderPrefix());

        if (oldGraph != nullptr)
            {
            const auto* oldHeatMap = dynamic_cast<const Graphs::HeatMap*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.continuous", GetContinuousVariable(),
                oldHeatMap != nullptr ? oldHeatMap->GetContinuousColumnName() : wxString{});
            const auto oldGroupName = (oldHeatMap != nullptr) ?
                                          oldHeatMap->GetGroupColumnName().value_or(wxString{}) :
                                          wxString{};
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.group",
                                               GetGroupVariable(), oldGroupName);
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.continuous", GetContinuousVariable());
            if (!GetGroupVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.group", GetGroupVariable());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
