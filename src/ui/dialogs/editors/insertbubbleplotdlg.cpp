///////////////////////////////////////////////////////////////////////////////
// Name:        insertbubbleplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertbubbleplotdlg.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/bubbleplot.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertBubblePlotDlg::InsertBubblePlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                             wxWindow* parent, const wxString& caption,
                                             const wxWindowID id, const wxPoint& pos,
                                             const wxSize& size, const long style,
                                             EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeShapeScheme)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    std::vector<DataGuideParameter> InsertBubblePlotDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return {
            { .m_name = _(L"X (independent)"),
              .m_types = { Data::Dataset::ColumnImportType::Numeric },
              .m_requirement = DataGuideRequirement::Required,
              .m_description = _(L"The measurement plotted along the X axis, such as a "
                                 "population."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Y (dependent)"),
              .m_types = { Data::Dataset::ColumnImportType::Numeric },
              .m_requirement = DataGuideRequirement::Required,
              .m_description = _(L"The measurement plotted along the Y axis, such as an average "
                                 "income."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Size (area)"),
              .m_types = { Data::Dataset::ColumnImportType::Numeric },
              .m_requirement = DataGuideRequirement::Required,
              .m_description = _(L"The measurement that controls how large each bubble is. The "
                                 "bubbles are scaled by area, with the smallest value getting "
                                 "the smallest bubble."),
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
              .m_description = _(L"Draws a separate series of bubbles, with its own regression "
                                 "line, for each unique value."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} }
        };
        }

    //-------------------------------------------
    DataGuide InsertBubblePlotDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Bubble Plot");
        guide.m_iconName = L"images/bubbleplot.svg";
        guide.m_summary =
            _(L"Shows the relationship between two measurements, like a scatter plot, with "
              "each point sized by a third measurement.");
        guide.m_requirements =
            _(L"One row per observation, holding its X value, Y value, and size value, and "
              "optionally the group that it belongs to.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample cities;
        cities.m_title = _(L"Example: cities by population, income, and number of schools");
        cities.m_columns = { { _(L"City"), wxString{} },
                             { _(L"Population"), _(L"X (independent)") },
                             { _(L"Income"), _(L"Y (dependent)") },
                             { _(L"Schools"), _(L"Size (area)") },
                             { _(L"Region"), _(L"Grouping") } };
        cities.m_rows = { { _(L"Akron"), L"190418", L"48544", L"45", _(L"East") },
                          { _(L"Dayton"), L"137625", L"43454", L"38", _(L"West") },
                          { _(L"Toledo"), L"265000", L"46000", L"60", _(L"West") },
                          { _(L"Canton"), L"70000", L"44000", L"22", _(L"East") } };
        guide.m_examples = { std::move(cities) };

        guide.m_missingData = {
            _(L"If the X, Y, or size value is missing, the point is left out of both the plot "
              "and the regression calculation."),
            _(L"A missing value in the grouping column is shown with an empty legend label.")
        };
        return guide;
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Bubble Plot"), ID_OPTIONS_SECTION, true);

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

        auto* sizeLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Size (area):"));
        sizeLabel->SetFont(sizeLabel->GetFont().Bold());
        varGrid->Add(sizeLabel, wxSizerFlags{}.CenterVertical());
        m_sizeVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_sizeVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_sizeVarLabel, wxSizerFlags{}.CenterVertical());

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

        // bubble sizing
        auto* bubbleSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        bubbleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Min bubble radius:")),
                         wxSizerFlags{}.CenterVertical());
            {
            auto* spin = new wxSpinCtrl(optionsPage, wxID_ANY);
            spin->SetRange(1, 100);
            spin->SetValue(m_minBubbleRadius);
            spin->SetValidator(wxGenericValidator{ &m_minBubbleRadius });
            bubbleSizer->Add(spin);
            }

        bubbleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Max bubble radius:")),
                         wxSizerFlags{}.CenterVertical());
            {
            auto* spin = new wxSpinCtrl(optionsPage, wxID_ANY);
            spin->SetRange(1, 200);
            spin->SetValue(m_maxBubbleRadius);
            spin->SetValidator(wxGenericValidator{ &m_maxBubbleRadius });
            bubbleSizer->Add(spin);
            }

        optionsSizer->Add(bubbleSizer, wxSizerFlags{}.Border());

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
    void InsertBubblePlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariable.clear();
        m_sizeVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::OnSelectVariables()
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

        const auto parameters = GetParameters();
        VariableSelectDlg dlg(
            this, columnInfo,
            { MakeVariableList(parameters[0], m_xVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_xVariable }),
              MakeVariableList(parameters[1], m_yVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_yVariable }),
              MakeVariableList(parameters[2], m_sizeVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_sizeVariable }),
              MakeVariableList(parameters[3], m_groupVariable.empty() ?
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

        const auto sizeVars = dlg.GetSelectedVariables(2);
        m_sizeVariable = sizeVars.empty() ? wxString{} : sizeVars.front();

        const auto groupVars = dlg.GetSelectedVariables(3);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::UpdateVariableLabels()
        {
        m_xVarLabel->SetLabel(m_xVariable);
        m_yVarLabel->SetLabel(m_yVariable);
        m_sizeVarLabel->SetLabel(m_sizeVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertBubblePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_xVariable.empty() || m_yVariable.empty() || m_sizeVariable.empty())
            {
            wxMessageBox(_(L"Please select the X, Y, and size variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
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
    void InsertBubblePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* bubble = dynamic_cast<const Graphs::BubblePlot*>(&graph);
        if (bubble == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(bubble->GetPropertyTemplate(L"dataset"));

        // load the actual column names used by the graph
        m_xVariable = bubble->GetXColumnName();
        m_yVariable = bubble->GetYColumnName();
        m_sizeVariable = bubble->GetSizeColumnName();
        const auto& series = bubble->GetSeriesList();
        if (!series.empty())
            {
            m_groupVariable = series.front().GetGroupColumnName().value_or(wxString{});
            }
        UpdateVariableLabels();

        // bubble-specific options
        m_showRegressionLines = bubble->IsShowingRegressionLines();
        m_showConfidenceBands = bubble->IsShowingConfidenceBands();
        m_confidenceLevel = bubble->GetConfidenceLevel();
        m_minBubbleRadius = static_cast<int>(bubble->GetMinBubbleRadius());
        m_maxBubbleRadius = static_cast<int>(bubble->GetMaxBubbleRadius());

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::BubblePlot>
    InsertBubblePlotDlg::BuildBubblePlot(const Graphs::Graph2D* oldGraph)
        {
        // the dialog does not edit the regression line styles,
        // so keep the ones from the plot being edited
        const auto* oldScatterPlot = dynamic_cast<const Graphs::ScatterPlot*>(oldGraph);
        auto plot = std::make_shared<Graphs::BubblePlot>(
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
        plot->SetMinBubbleRadius(GetMinBubbleRadius());
        plot->SetMaxBubbleRadius(GetMaxBubbleRadius());

        const std::optional<wxString> groupCol =
            GetGroupVariable().empty() ? std::nullopt : std::optional<wxString>(GetGroupVariable());
        plot->SetData(GetSelectedDataset(), GetYVariable(), GetXVariable(), GetSizeVariable(),
                      groupCol);

        ApplyAxisOverrides(*plot);

        if (oldGraph != nullptr)
            {
            const auto* oldBubble = dynamic_cast<const Graphs::BubblePlot*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.x", GetXVariable(),
                                               oldBubble != nullptr ? oldBubble->GetXColumnName() :
                                                                      wxString{});
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.y", GetYVariable(),
                                               oldBubble != nullptr ? oldBubble->GetYColumnName() :
                                                                      wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.size", GetSizeVariable(),
                oldBubble != nullptr ? oldBubble->GetSizeColumnName() : wxString{});
            const auto oldGroupName =
                (oldBubble != nullptr && !oldBubble->GetSeriesList().empty()) ?
                    oldBubble->GetSeriesList().front().GetGroupColumnName().value_or(wxString{}) :
                    wxString{};
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.group",
                                               GetGroupVariable(), oldGroupName);
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.y", GetYVariable());
            plot->SetPropertyTemplate(L"variables.x", GetXVariable());
            plot->SetPropertyTemplate(L"variables.size", GetSizeVariable());
            if (!GetGroupVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.group", GetGroupVariable());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
