///////////////////////////////////////////////////////////////////////////////
// Name:        insertduelingpiechartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertduelingpiechartdlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertDuelingPieChartDlg::InsertDuelingPieChartDlg(Canvas* canvas,
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
    std::vector<DataGuideParameter> InsertDuelingPieChartDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"Category"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The categories that become the slices in each pie, such as "
                                      "activities."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Group"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The group that each value belongs to. Each group gets its "
                                      "own pie, and there must be exactly two groups."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Value"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The amount for each category within each group, such as "
                                      "hours."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertDuelingPieChartDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Dueling Pie Chart");
        guide.m_iconName = L"images/dueling-pie.svg";
        guide.m_summary =
            _(L"Compares how two groups divide a whole, with a pie for each group using the "
              "same categories for the slices.");
        guide.m_requirements =
            _(L"One row per category per group, holding the category, the group, and the "
              "value.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample hours;
        hours.m_title = _(L"Example: how a weekday and a weekend day are spent");
        hours.m_columns = { { _(L"Activity"), _(L"Category") },
                            { _(L"Day"), _(L"Group") },
                            { _(L"Hours"), _(L"Value") } };
        hours.m_rows = { { _(L"Work"), _(L"Weekday"), L"8" },
                         { _(L"Sleep"), _(L"Weekday"), L"7" },
                         { _(L"Leisure"), _(L"Weekday"), L"4" },
                         { _(L"Work"), _(L"Weekend"), L"1" },
                         { _(L"Sleep"), _(L"Weekend"), L"9" } };
        guide.m_examples = { std::move(hours) };

        guide.m_missingData = {
            _(L"A row with a missing or non-positive value is ignored."),
            _(L"A row with a missing group is ignored."),
            _(L"A row with a missing category is not drawn."),
            _(L"If a group has no value for a category, then that group's pie has no slice "
              "for it. The category keeps its color and legend entry.")
        };
        return guide;
        }

    //-------------------------------------------
    void InsertDuelingPieChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Dueling Pie Chart"), ID_OPTIONS_SECTION, true);

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

        const auto addVariableRow = [&](const wxString& caption, wxStaticText*& valueLabel)
        {
            auto* captionLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, caption);
            captionLabel->SetFont(captionLabel->GetFont().Bold());
            varGrid->Add(captionLabel, wxSizerFlags{}.CenterVertical());
            valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
            valueLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
            varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        };
        addVariableRow(_(L"Category:"), m_categoryVarLabel);
        addVariableRow(_(L"Group:"), m_groupVarLabel);
        addVariableRow(_(L"Value:"), m_valueVarLabel);

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // display options
        auto* layoutSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Slice labels:")),
                         wxSizerFlags{}.CenterVertical());

        layoutSizer->Add(
            new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                         wxArrayString{ _(L"Percentage"), _(L"Value"), _(L"Value and percentage") },
                         0, wxGenericValidator{ &m_midPointLabelIndex }));

        optionsSizer->Add(layoutSizer, wxSizerFlags{}.Border());

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    BinLabelDisplay InsertDuelingPieChartDlg::GetMidPointLabelDisplay() const noexcept
        {
        switch (m_midPointLabelIndex)
            {
        case 1:
            return BinLabelDisplay::BinValue;
        case 2:
            return BinLabelDisplay::BinValueAndPercentage;
        default:
            return BinLabelDisplay::BinPercentage;
            }
        }

    //-------------------------------------------
    void InsertDuelingPieChartDlg::OnDatasetChanged()
        {
        m_categoryVariable.clear();
        m_groupVariable.clear();
        m_valueVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertDuelingPieChartDlg::OnSelectVariables()
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
            { MakeVariableList(parameters[0], m_categoryVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_categoryVariable }),
              MakeVariableList(parameters[1], m_groupVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_groupVariable }),
              MakeVariableList(parameters[2], m_valueVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_valueVariable }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto categoryVars = dlg.GetSelectedVariables(0);
        m_categoryVariable = categoryVars.empty() ? wxString{} : categoryVars.front();

        const auto groupVars = dlg.GetSelectedVariables(1);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        const auto valueVars = dlg.GetSelectedVariables(2);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertDuelingPieChartDlg::UpdateVariableLabels()
        {
        m_categoryVarLabel->SetLabel(m_categoryVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);
        m_valueVarLabel->SetLabel(m_valueVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertDuelingPieChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_categoryVariable.empty() || m_groupVariable.empty() || m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the category, group, and value variables."),
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
    void InsertDuelingPieChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* duelingChart = dynamic_cast<const Graphs::DuelingPieChart*>(&graph);
        if (duelingChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(duelingChart->GetPropertyTemplate(L"dataset"));

        // load column names from the graph
        m_categoryVariable = duelingChart->GetCategoryColumnName();
        m_groupVariable = duelingChart->GetGroupColumnName();
        m_valueVariable = duelingChart->GetValueColumnName();
        UpdateVariableLabels();

        switch (duelingChart->GetMidPointLabelDisplay())
            {
        case BinLabelDisplay::BinValue:
            m_midPointLabelIndex = 1;
            break;
        case BinLabelDisplay::BinValueAndPercentage:
            m_midPointLabelIndex = 2;
            break;
        default:
            m_midPointLabelIndex = 0;
            break;
            }

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::DuelingPieChart>
    InsertDuelingPieChartDlg::BuildDuelingPieChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::DuelingPieChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        // sets the color scheme, which the chart applies to the slices when it lays itself out
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetMidPointLabelDisplay(GetMidPointLabelDisplay());

        plot->SetData(GetSelectedDataset(), GetValueVariable(), GetCategoryVariable(),
                      GetGroupVariable());

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::DuelingPieChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldChart != nullptr ? oldChart->GetValueColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.category", GetCategoryVariable(),
                oldChart != nullptr ? oldChart->GetCategoryColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.group", GetGroupVariable(),
                oldChart != nullptr ? oldChart->GetGroupColumnName() : wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            plot->SetPropertyTemplate(L"variables.category", GetCategoryVariable());
            plot->SetPropertyTemplate(L"variables.group", GetGroupVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
