///////////////////////////////////////////////////////////////////////////////
// Name:        insertscatterplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertscatterplotdlg.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/scatterplot.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertScatterPlotDlg::InsertScatterPlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeAll)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    std::vector<DataGuideParameter> InsertScatterPlotDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"X (independent)"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The measurement plotted along the X axis, such as hours "
                                      "studied."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Y (dependent)"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The measurement plotted along the Y axis, such as an exam "
                                      "score."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Grouping"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Optional,
                   .m_description = _(L"Draws a separate series of points, with its own regression "
                                      "line, for each unique value."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertScatterPlotDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Scatter Plot");
        guide.m_iconName = L"images/scatterplot.svg";
        guide.m_summary = _(L"Shows the relationship between two measurements, with one point per "
                            "observation and an optional regression line for each group.");
        guide.m_requirements =
            _(L"One row per observation, holding its X value, its Y value, and optionally the "
              "group that it belongs to.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample study;
        study.m_title = _(L"Example: hours studied versus exam score for two sections");
        study.m_columns = { { _(L"Hours"), _(L"X (independent)") },
                            { _(L"Score"), _(L"Y (dependent)") },
                            { _(L"Section"), _(L"Grouping") } };
        study.m_rows = { { L"2", L"68", L"A" }, { L"4", L"75", L"A" }, { L"6", L"84", L"A" },
                         { L"1", L"60", L"B" }, { L"3", L"71", L"B" }, { L"5", L"79", L"B" } };
        guide.m_examples = { std::move(study) };

        guide.m_missingData = {
            _(L"If either the X or Y value is missing, the point is left out of both the plot "
              "and the regression calculation."),
            _(L"A missing value in the grouping column is shown with an empty legend label.")
        };
        return guide;
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Scatter Plot"), ID_OPTIONS_SECTION, true);

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

        auto* xLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"X (independent):"));
        xLabel->SetFont(xLabel->GetFont().Bold());
        varGrid->Add(xLabel, wxSizerFlags{}.CenterVertical());
        m_xVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_xVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_xVarLabel, wxSizerFlags{}.CenterVertical());

        auto* yLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Y (dependent):"));
        yLabel->SetFont(yLabel->GetFont().Bold());
        varGrid->Add(yLabel, wxSizerFlags{}.CenterVertical());
        m_yVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_yVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_yVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // regression options
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show regression lines"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showRegressionLines }),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show confidence bands"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showConfidenceBands }),
                          wxSizerFlags{}.Border());

        auto* confidenceSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        confidenceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Confidence level:")),
                             wxSizerFlags{}.CenterVertical());
            {
            m_confidenceLevelSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY);
            m_confidenceLevelSpin->SetRange(0.5, 0.99);
            m_confidenceLevelSpin->SetDigits(2);
            m_confidenceLevelSpin->SetIncrement(0.01);
            // validators do not work with wxSpinCtrlDouble
            m_confidenceLevelSpin->SetValue(m_confidenceLevel);
            confidenceSizer->Add(m_confidenceLevelSpin);
            }
        optionsSizer->Add(confidenceSizer, wxSizerFlags{}.Border());

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage();
        CreateAnnotationsPage();
        CreateAxisOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        // prefer the stored column preview info (preserves original file order)
        // over rebuilding it from the dataset's internal column grouping
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

        const auto parameters = GetParameters();
        VariableSelectDlg dlg(
            this, columnInfo,
            { MakeVariableList(parameters[0], m_xVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_xVariable }),
              MakeVariableList(parameters[1], m_yVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_yVariable }),
              MakeVariableList(parameters[2], m_groupVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_groupVariable }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto xVars = dlg.GetSelectedVariables(0);
        m_xVariable = xVars.empty() ? wxString{} : xVars.front();

        const auto yVars = dlg.GetSelectedVariables(1);
        m_yVariable = yVars.empty() ? wxString{} : yVars.front();

        const auto groupVars = dlg.GetSelectedVariables(2);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::UpdateVariableLabels()
        {
        m_xVarLabel->SetLabel(m_xVariable);
        m_yVarLabel->SetLabel(m_yVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertScatterPlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_xVariable.empty() || m_yVariable.empty())
            {
            wxMessageBox(_(L"Please select the X and Y variables."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        if (!ValidateColorScheme())
            {
            return false;
            }

        m_confidenceLevel = m_confidenceLevelSpin->GetValue();

        return true;
        }

    //-------------------------------------------
    void InsertScatterPlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* scatter = dynamic_cast<const Graphs::ScatterPlot*>(&graph);
        if (scatter == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(scatter->GetPropertyTemplate(L"dataset"));

        // load the actual column names used by the graph
        // (property templates may contain unexpanded {{placeholders}})
        m_xVariable = scatter->GetXColumnName();
        m_yVariable = scatter->GetYColumnName();
        const auto& series = scatter->GetSeriesList();
        if (!series.empty())
            {
            m_groupVariable = series.front().GetGroupColumnName().value_or(wxString{});
            }
        UpdateVariableLabels();

        // scatter-specific options
        m_showRegressionLines = scatter->IsShowingRegressionLines();
        m_showConfidenceBands = scatter->IsShowingConfidenceBands();
        m_confidenceLevel = scatter->GetConfidenceLevel();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::ScatterPlot>
    InsertScatterPlotDlg::BuildScatterPlot(const Graphs::Graph2D* oldGraph)
        {
        // the dialog does not edit the regression line styles,
        // so keep the ones from the plot being edited
        const auto* oldScatterPlot = dynamic_cast<const Graphs::ScatterPlot*>(oldGraph);
        auto plot = std::make_shared<Graphs::ScatterPlot>(
            GetCanvas(), nullptr, nullptr,
            (oldScatterPlot != nullptr) ? oldScatterPlot->GetRegressionLineStyleScheme() : nullptr);
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);
        plot->ShowRegressionLines(GetShowRegressionLines());
        plot->ShowConfidenceBands(GetShowConfidenceBands());
        plot->SetConfidenceLevel(GetConfidenceLevel());
        plot->SetShapeScheme(GetShapeScheme());

        const std::optional<wxString> groupCol =
            GetGroupVariable().empty() ? std::nullopt : std::optional<wxString>(GetGroupVariable());
        plot->SetData(GetSelectedDataset(), GetYVariable(), GetXVariable(), groupCol);

        ApplyAxisOverrides(*plot);

        if (oldGraph != nullptr)
            {
            const auto* oldScatter = dynamic_cast<const Graphs::ScatterPlot*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.x", GetXVariable(),
                oldScatter != nullptr ? oldScatter->GetXColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.y", GetYVariable(),
                oldScatter != nullptr ? oldScatter->GetYColumnName() : wxString{});
            const auto oldGroupName =
                (oldScatter != nullptr && !oldScatter->GetSeriesList().empty()) ?
                    oldScatter->GetSeriesList().front().GetGroupColumnName().value_or(wxString{}) :
                    wxString{};
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.group",
                                               GetGroupVariable(), oldGroupName);
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.y", GetYVariable());
            plot->SetPropertyTemplate(L"variables.x", GetXVariable());
            if (!GetGroupVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.group", GetGroupVariable());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
