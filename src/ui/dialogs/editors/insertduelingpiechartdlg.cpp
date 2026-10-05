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

        const std::vector<Data::Dataset::ColumnImportType> categoricalTypes{
            Data::Dataset::ColumnImportType::String, Data::Dataset::ColumnImportType::Discrete,
            Data::Dataset::ColumnImportType::DichotomousString,
            Data::Dataset::ColumnImportType::DichotomousDiscrete
        };

        using VLI = VariableSelectDlg::VariableListInfo;
        VariableSelectDlg dlg(
            this, columnInfo,
            { VLI{}
                  .Label(_(L"Category"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_categoryVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_categoryVariable })
                  .AcceptedTypes(categoricalTypes),
              VLI{}
                  .Label(_(L"Group"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
                  .AcceptedTypes(categoricalTypes),
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
