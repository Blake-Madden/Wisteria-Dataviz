///////////////////////////////////////////////////////////////////////////////
// Name:        insert_run_chart_dlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insert_run_chart_dlg.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/runchart.h"
#include "../variableselectdlg.h"
#include <wx/spinctrl.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertRunChartDlg::InsertRunChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                         wxWindow* parent, const wxString& caption,
                                         const wxWindowID id, const wxPoint& pos,
                                         const wxSize& size, const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeAll)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    std::vector<DataGuideParameter> InsertRunChartDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"Value"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The measurement being tracked over time."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Sample"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::Numeric,
                                Data::Dataset::ColumnImportType::DichotomousString,
                                Data::Dataset::ColumnImportType::DichotomousDiscrete },
                   .m_requirement = DataGuideRequirement::Conditional,
                   .m_description = _(L"Identifies which sample each row belongs to. A new sample "
                                      "starts whenever the value changes from the previous row. "
                                      "A numeric column must contain whole numbers."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = _(L"Needed when collecting rows into samples by a variable."),
                   .m_group = wxString{} },
                 { .m_name = _(L"Label"),
                   .m_types = { Data::Dataset::ColumnImportType::String,
                                Data::Dataset::ColumnImportType::Discrete,
                                Data::Dataset::ColumnImportType::Numeric,
                                Data::Dataset::ColumnImportType::Date },
                   .m_requirement = DataGuideRequirement::Optional,
                   .m_description =
                       _(L"The text shown along the x-axis, such as a date or batch name. "
                         "This does not change the order or spacing of the points."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertRunChartDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Run Chart");
        guide.m_iconName = L"images/runchart.svg";
        guide.m_summary = _(L"Shows measurements in the order they were taken, with a median line, "
                            "to reveal trends, shifts, and other patterns over time.");
        guide.m_requirements =
            _(L"One row per measurement, in the order that they were collected.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample readings;
        readings.m_title = _(L"Example: two readings taken each day");
        readings.m_columns = { { _(L"Sample"), _(L"Sample") },
                               { _(L"Radon"), _(L"Value") },
                               { _(L"Day"), _(L"Label") } };
        readings.m_rows = { { L"1", L"4.1", L"Mon" }, { L"1", L"4.5", L"Mon" },
                            { L"2", L"3.8", L"Tue" }, { L"2", L"4.0", L"Tue" },
                            { L"3", L"5.2", L"Wed" }, { L"3", L"4.9", L"Wed" } };
        guide.m_examples = { std::move(readings) };

        guide.m_missingData = { _(
            L"A missing value is left as a gap in the line and is not used in the median. "
            "A sample with no values is also drawn as a gap.") };
        guide.m_notes = { _(L"The order of the rows is the order of the points. Rows are never "
                            "sorted. The same sample value in rows that are not next to each "
                            "other starts a new sample.") };
        return guide;
        }

    //-------------------------------------------
    void InsertRunChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Run Chart"), ID_OPTIONS_SECTION, true);

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

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Value:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        auto* sampleLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Sample:"));
        sampleLabel->SetFont(sampleLabel->GetFont().Bold());
        varGrid->Add(sampleLabel, wxSizerFlags{}.CenterVertical());
        m_sampleVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_sampleVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_sampleVarLabel, wxSizerFlags{}.CenterVertical());

        auto* labelLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Label:"));
        labelLabel->SetFont(labelLabel->GetFont().Bold());
        varGrid->Add(labelLabel, wxSizerFlags{}.CenterVertical());
        m_labelVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_labelVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_labelVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // samples
        auto* sampleBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Samples"));
        auto* sampleGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        sampleGrid->Add(new wxStaticText(sampleBox->GetStaticBox(), wxID_ANY, _(L"Collect:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString modeChoices;
        modeChoices.Add(_(L"Each row is a point"));
        modeChoices.Add(_(L"Rows sharing the Sample value"));
        modeChoices.Add(_(L"A fixed number of rows per sample"));
        auto* modeChoice =
            new wxChoice(sampleBox->GetStaticBox(), ID_SAMPLE_MODE, wxDefaultPosition,
                         wxDefaultSize, modeChoices, 0, wxGenericValidator{ &m_sampleMode });
        sampleGrid->Add(modeChoice);

        sampleGrid->Add(
            new wxStaticText(sampleBox->GetStaticBox(), wxID_ANY, _(L"Rows per sample:")),
            wxSizerFlags{}.CenterVertical());
        m_sampleSizeSpin = new wxSpinCtrl(sampleBox->GetStaticBox(), wxID_ANY);
        m_sampleSizeSpin->SetRange(1, 1000);
        m_sampleSizeSpin->SetValidator(wxGenericValidator{ &m_sampleSize });
        sampleGrid->Add(m_sampleSizeSpin);

        sampleGrid->Add(new wxStaticText(sampleBox->GetStaticBox(), wxID_ANY, _(L"Plot:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString statisticChoices;
        statisticChoices.Add(_(L"Sample mean"));
        statisticChoices.Add(_(L"Sample median"));
        m_statisticChoice =
            new wxChoice(sampleBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                         statisticChoices, 0, wxGenericValidator{ &m_sampleStatistic });
        sampleGrid->Add(m_statisticChoice);

        sampleBox->Add(sampleGrid, wxSizerFlags{}.Border());

        m_individualPointsCheck = new wxCheckBox(
            sampleBox->GetStaticBox(), wxID_ANY, _(L"Show individual observations"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_showIndividualPoints });
        sampleBox->Add(m_individualPointsCheck, wxSizerFlags{}.Border());

        auto* opacitySizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        opacitySizer->Add(
            new wxStaticText(sampleBox->GetStaticBox(), wxID_ANY, _(L"Observation opacity:")),
            wxSizerFlags{}.CenterVertical());
        m_individualOpacitySpin = new wxSpinCtrl(sampleBox->GetStaticBox(), wxID_ANY);
        m_individualOpacitySpin->SetRange(0, 255);
        m_individualOpacitySpin->SetValidator(wxGenericValidator{ &m_individualPointOpacity });
        opacitySizer->Add(m_individualOpacitySpin);
        sampleBox->Add(opacitySizer, wxSizerFlags{}.Border());

        optionsSizer->Add(sampleBox, wxSizerFlags{}.Border());

        // median line
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show median line"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showMedianLine }),
                          wxSizerFlags{}.Border());

        // baseline
        auto* baselineSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        // the label is set by UpdateSampleControls(), based on whether samples are used
        m_baselineCheck = new wxCheckBox(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, 0, wxGenericValidator{ &m_useBaseline });
        baselineSizer->Add(m_baselineCheck, wxSizerFlags{}.CenterVertical());
        m_baselineLengthSpin = new wxSpinCtrl(optionsPage, wxID_ANY);
        m_baselineLengthSpin->SetRange(1, 100'000);
        m_baselineLengthSpin->SetValidator(wxGenericValidator{ &m_baselineLength });
        baselineSizer->Add(m_baselineLengthSpin);
        optionsSizer->Add(baselineSizer, wxSizerFlags{}.Border());

        // rules
        auto* rulesBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Rules"));
        auto* rulesGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        const auto addLengthRule = [&rulesBox, &rulesGrid](const wxString& label, bool* useRule,
                                                           int* length, wxSpinCtrl*& lengthSpin,
                                                           const size_t smallest)
        {
            auto* ruleCheck =
                new wxCheckBox(rulesBox->GetStaticBox(), wxID_ANY, label, wxDefaultPosition,
                               wxDefaultSize, 0, wxGenericValidator{ useRule });
            rulesGrid->Add(ruleCheck, wxSizerFlags{}.CenterVertical());
            lengthSpin = new wxSpinCtrl(rulesBox->GetStaticBox(), wxID_ANY);
            lengthSpin->SetRange(static_cast<int>(smallest), 1000);
            lengthSpin->SetValidator(wxGenericValidator{ length });
            rulesGrid->Add(lengthSpin);
            return ruleCheck;
        };

        auto* shiftCheck = addLengthRule(_(L"Shift (points on one side of the median):"),
                                         &m_useShiftRule, &m_shiftLength, m_shiftLengthSpin,
                                         statistics::spc_rules::MIN_SHIFT_LENGTH);
        auto* trendCheck = addLengthRule(_(L"Trend (points all rising or all falling):"),
                                         &m_useTrendRule, &m_trendLength, m_trendLengthSpin,
                                         statistics::spc_rules::MIN_TREND_LENGTH);
        auto* alternatingCheck =
            addLengthRule(_(L"Alternating (points going up and down):"), &m_useAlternatingRule,
                          &m_alternatingLength, m_alternatingLengthSpin,
                          statistics::spc_rules::MIN_ALTERNATING_LENGTH);
        rulesBox->Add(rulesGrid, wxSizerFlags{}.Border());

        rulesBox->Add(new wxCheckBox(rulesBox->GetStaticBox(), wxID_ANY,
                                     _(L"Too few or too many runs about the median"),
                                     wxDefaultPosition, wxDefaultSize, 0,
                                     wxGenericValidator{ &m_useRunsRule }),
                      wxSizerFlags{}.Border());

        optionsSizer->Add(rulesBox, wxSizerFlags{}.Border());

        // bind events
        for (auto* ruleCheck : { shiftCheck, trendCheck, alternatingCheck })
            {
            ruleCheck->Bind(wxEVT_CHECKBOX,
                            [this]([[maybe_unused]] wxCommandEvent&)
                            {
                                TransferDataFromWindow();
                                UpdateRuleControls();
                            });
            }

        m_baselineCheck->Bind(wxEVT_CHECKBOX,
                              [this]([[maybe_unused]] wxCommandEvent&)
                              {
                                  TransferDataFromWindow();
                                  UpdateBaselineControls();
                              });

        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        // these change the number of points, the axis title, or the y-range,
        // so any axes loaded from an existing chart are stale
        modeChoice->Bind(wxEVT_CHOICE,
                         [this]([[maybe_unused]] wxCommandEvent&)
                         {
                             TransferDataFromWindow();
                             UpdateSampleControls();
                             ResetAxisOptions();
                         });
        m_sampleSizeSpin->Bind(wxEVT_SPINCTRL,
                               [this]([[maybe_unused]] wxSpinEvent&) { ResetAxisOptions(); });
        m_statisticChoice->Bind(wxEVT_CHOICE,
                                [this]([[maybe_unused]] wxCommandEvent&) { ResetAxisOptions(); });
        m_individualPointsCheck->Bind(wxEVT_CHECKBOX, [this]([[maybe_unused]] wxCommandEvent&)
                                      { ResetAxisOptions(); });

        UpdateSampleControls();
        UpdateRuleControls();
        UpdateBaselineControls();

        CreateLegendOptionsPage();
        CreateAnnotationsPage();
        CreateAxisOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertRunChartDlg::UpdateSampleControls()
        {
        if (m_sampleSizeSpin != nullptr)
            {
            m_sampleSizeSpin->Enable(m_sampleMode == static_cast<int>(SampleMode::FixedSize));
            }
        // the statistic and individual observations only apply when collecting samples
        const bool isCollecting{ m_sampleMode != static_cast<int>(SampleMode::None) };
        if (m_statisticChoice != nullptr)
            {
            m_statisticChoice->Enable(isCollecting);
            }
        if (m_individualPointsCheck != nullptr)
            {
            m_individualPointsCheck->Enable(isCollecting);
            }
        if (m_individualOpacitySpin != nullptr)
            {
            m_individualOpacitySpin->Enable(isCollecting);
            }
        // the baseline counts plotted points, which are samples when collecting samples
        if (m_baselineCheck != nullptr)
            {
            m_baselineCheck->SetLabel(
                isCollecting ? _(L"Compute the median from the first (baseline) samples:") :
                               _(L"Compute the median from the first (baseline) observations:"));
            m_baselineCheck->GetParent()->Layout();
            }
        }

    //-------------------------------------------
    void InsertRunChartDlg::UpdateBaselineControls()
        {
        if (m_baselineLengthSpin != nullptr)
            {
            m_baselineLengthSpin->Enable(m_useBaseline);
            }
        }

    //-------------------------------------------
    void InsertRunChartDlg::UpdateRuleControls()
        {
        if (m_shiftLengthSpin != nullptr)
            {
            m_shiftLengthSpin->Enable(m_useShiftRule);
            }
        if (m_trendLengthSpin != nullptr)
            {
            m_trendLengthSpin->Enable(m_useTrendRule);
            }
        if (m_alternatingLengthSpin != nullptr)
            {
            m_alternatingLengthSpin->Enable(m_useAlternatingRule);
            }
        }

    //-------------------------------------------
    // choosing a sample variable collects by it, and clearing it stops collecting by it
    void InsertRunChartDlg::SyncSampleModeToVariable()
        {
        TransferDataFromWindow();
        if (!m_sampleVariable.empty() && m_sampleMode == static_cast<int>(SampleMode::None))
            {
            m_sampleMode = static_cast<int>(SampleMode::ByVariable);
            }
        else if (m_sampleVariable.empty() &&
                 m_sampleMode == static_cast<int>(SampleMode::ByVariable))
            {
            m_sampleMode = static_cast<int>(SampleMode::None);
            }
        TransferDataToWindow();
        UpdateSampleControls();
        }

    //-------------------------------------------
    void InsertRunChartDlg::OnDatasetChanged()
        {
        m_valueVariable.clear();
        m_labelVariable.clear();
        m_sampleVariable.clear();
        UpdateVariableLabels();
        SyncSampleModeToVariable();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertRunChartDlg::OnSelectVariables()
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
            { MakeVariableList(parameters[0], m_valueVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_valueVariable }),
              MakeVariableList(parameters[1], m_sampleVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_sampleVariable }),
              MakeVariableList(parameters[2], m_labelVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_labelVariable }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto valueVars = dlg.GetSelectedVariables(0);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        const auto sampleVars = dlg.GetSelectedVariables(1);
        m_sampleVariable = sampleVars.empty() ? wxString{} : sampleVars.front();

        const auto labelVars = dlg.GetSelectedVariables(2);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        UpdateVariableLabels();
        SyncSampleModeToVariable();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertRunChartDlg::UpdateVariableLabels()
        {
        m_valueVarLabel->SetLabel(m_valueVariable);
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_sampleVarLabel->SetLabel(m_sampleVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertRunChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the value variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        if (m_sampleMode == static_cast<int>(SampleMode::ByVariable) && m_sampleVariable.empty())
            {
            wxMessageBox(_(L"Please select the sample variable, or choose a different way "
                           "to collect the rows."),
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
    void InsertRunChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* runChart = dynamic_cast<const Graphs::RunChart*>(&graph);
        if (runChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(runChart->GetPropertyTemplate(L"dataset"));

        // load the actual column names from the graph
        m_valueVariable = runChart->GetYColumnName();
        m_labelVariable = runChart->GetLabelColumnName().value_or(wxString{});
        m_sampleVariable = runChart->GetSampleColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // run-chart-specific options
        if (runChart->GetSampleColumnName().has_value())
            {
            m_sampleMode = static_cast<int>(SampleMode::ByVariable);
            }
        else if (runChart->GetSampleSize().has_value())
            {
            m_sampleMode = static_cast<int>(SampleMode::FixedSize);
            m_sampleSize = static_cast<int>(runChart->GetSampleSize().value());
            }
        else
            {
            m_sampleMode = static_cast<int>(SampleMode::None);
            }
        m_sampleStatistic =
            (runChart->GetSampleStatistic() == Graphs::SampleStatistic::Median) ? 1 : 0;
        m_showMedianLine = runChart->IsShowingMedianLine();
        m_useBaseline = (runChart->GetBaselineLength() > 0);
        if (m_useBaseline)
            {
            m_baselineLength = static_cast<int>(runChart->GetBaselineLength());
            }
        m_showIndividualPoints = runChart->IsShowingIndividualPoints();
        m_individualPointOpacity = runChart->GetIndividualPointOpacity();

        m_useShiftRule = runChart->IsRuleEnabled(statistics::spc_rule::shift);
        m_shiftLength = static_cast<int>(runChart->GetShiftLength());
        m_useTrendRule = runChart->IsRuleEnabled(statistics::spc_rule::trend);
        m_trendLength = static_cast<int>(runChart->GetTrendLength());
        m_useAlternatingRule = runChart->IsRuleEnabled(statistics::spc_rule::alternating);
        m_alternatingLength = static_cast<int>(runChart->GetAlternatingLength());
        m_useRunsRule = runChart->IsRuleEnabled(statistics::spc_rule::runs);

        TransferDataToWindow();
        UpdateSampleControls();
        UpdateRuleControls();
        UpdateBaselineControls();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::RunChart>
    InsertRunChartDlg::BuildRunChart(const Graphs::Graph2D* oldGraph)
        {
        // the dialog does not edit the line styles, so keep the ones from the chart being edited
        const auto* oldLinePlot = dynamic_cast<const Graphs::LinePlot*>(oldGraph);
        auto plot = std::make_shared<Graphs::RunChart>(
            GetCanvas(), nullptr, nullptr,
            (oldLinePlot != nullptr) ? oldLinePlot->GetLineStyleScheme() : nullptr);
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);
        plot->SetShapeScheme(GetShapeScheme());

        const std::optional<wxString> labelColumn =
            GetLabelVariable().empty() ? std::nullopt : std::optional<wxString>(GetLabelVariable());
        const auto statistic = (m_sampleStatistic == 1) ? Graphs::SampleStatistic::Median :
                                                          Graphs::SampleStatistic::Mean;

        if (m_sampleMode == static_cast<int>(SampleMode::ByVariable))
            {
            plot->SetSampleData(GetSelectedDataset(), GetValueVariable(), GetSampleVariable(),
                                labelColumn, statistic);
            }
        else if (m_sampleMode == static_cast<int>(SampleMode::FixedSize))
            {
            plot->SetSampleData(GetSelectedDataset(), GetValueVariable(),
                                static_cast<size_t>(m_sampleSize), labelColumn, statistic);
            }
        else
            {
            plot->SetData(GetSelectedDataset(), GetValueVariable(), labelColumn);
            }
        plot->ShowMedianLine(m_showMedianLine);
        plot->SetBaselineLength(m_useBaseline ? static_cast<size_t>(m_baselineLength) : 0);
        plot->ShowIndividualPoints(m_showIndividualPoints);
        plot->SetIndividualPointOpacity(static_cast<uint8_t>(m_individualPointOpacity));
        plot->SetShiftLength(static_cast<size_t>(m_shiftLength));
        plot->EnableRule(statistics::spc_rule::shift, m_useShiftRule);
        plot->SetTrendLength(static_cast<size_t>(m_trendLength));
        plot->EnableRule(statistics::spc_rule::trend, m_useTrendRule);
        plot->SetAlternatingLength(static_cast<size_t>(m_alternatingLength));
        plot->EnableRule(statistics::spc_rule::alternating, m_useAlternatingRule);
        plot->EnableRule(statistics::spc_rule::runs, m_useRunsRule);
        // the dialog does not edit the rule colors, so keep the ones from the chart being edited
        if (const auto* oldRuleChart = dynamic_cast<const Graphs::SpcChart*>(oldGraph);
            oldRuleChart != nullptr)
            {
            for (const auto rule : { statistics::spc_rule::shift, statistics::spc_rule::trend,
                                     statistics::spc_rule::alternating })
                {
                plot->SetRuleColor(rule, oldRuleChart->GetRuleColor(rule));
                }
            }
        ApplyAxisOverrides(*plot);

        const bool usesSampleVariable = (m_sampleMode == static_cast<int>(SampleMode::ByVariable));
        if (oldGraph != nullptr)
            {
            const auto* oldRunChart = dynamic_cast<const Graphs::RunChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldRunChart != nullptr ? oldRunChart->GetYColumnName() : wxString{});
            if (!GetLabelVariable().empty())
                {
                WisteriaView::CarryForwardProperty(
                    *oldGraph, *plot, L"variables.label", GetLabelVariable(),
                    oldRunChart != nullptr ?
                        oldRunChart->GetLabelColumnName().value_or(wxString{}) :
                        wxString{});
                }
            if (usesSampleVariable)
                {
                WisteriaView::CarryForwardProperty(
                    *oldGraph, *plot, L"variables.sample", GetSampleVariable(),
                    oldRunChart != nullptr ?
                        oldRunChart->GetSampleColumnName().value_or(wxString{}) :
                        wxString{});
                }
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            if (!GetLabelVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.label", GetLabelVariable());
                }
            if (usesSampleVariable)
                {
                plot->SetPropertyTemplate(L"variables.sample", GetSampleVariable());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
