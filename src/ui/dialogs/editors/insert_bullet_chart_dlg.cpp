///////////////////////////////////////////////////////////////////////////////
// Name:        insert_bullet_chart_dlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insert_bullet_chart_dlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    // range edit sub-dialog
    //----------------------
    class EditRangeDlg : public wxDialog
        {
      public:
        EditRangeDlg(wxWindow* parent, const Graphs::BulletChart::Range& range)
            : wxDialog(parent, wxID_ANY, _(L"Edit Range"), wxDefaultPosition, wxDefaultSize,
                       wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
              m_label(range.m_label), m_end(range.m_end)
            {
            auto* mainSizer = new wxBoxSizer(wxVERTICAL);

            auto* grid = new wxFlexGridSizer(2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                        wxSizerFlags::GetDefaultBorder() });
            grid->AddGrowableCol(1, 1);

            grid->Add(new wxStaticText(this, wxID_ANY, _(L"Label:")),
                      wxSizerFlags{}.CenterVertical());
            grid->Add(new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                     wxSize{ FromDIP(200), -1 }, 0,
                                     wxTextValidator{ wxFILTER_NONE, &m_label }),
                      wxSizerFlags{}.Expand());

            grid->Add(new wxStaticText(this, wxID_ANY, _(L"End value:")),
                      wxSizerFlags{}.CenterVertical());
            m_endCtrl = new wxSpinCtrlDouble(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                             wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9, m_end, 1);
            m_endCtrl->SetDigits(2);
            grid->Add(m_endCtrl);

            mainSizer->Add(grid, wxSizerFlags{ 1 }.Expand().Border());

            auto* btnSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
            mainSizer->Add(btnSizer, wxSizerFlags{}.Expand().Border());

            SetSizerAndFit(mainSizer);
            Centre();
            }

        [[nodiscard]]
        Graphs::BulletChart::Range GetRange() const
            {
            return { m_endCtrl->GetValue(), m_label };
            }

      private:
        wxString m_label;
        double m_end{ 0 };
        wxSpinCtrlDouble* m_endCtrl{ nullptr };
        };

    //-------------------------------------------
    InsertBulletChartDlg::InsertBulletChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeNone)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertBulletChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Bullet Chart"), ID_OPTIONS_SECTION, true);

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

        auto* actualLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Actual:"));
        actualLabel->SetFont(actualLabel->GetFont().Bold());
        varGrid->Add(actualLabel, wxSizerFlags{}.CenterVertical());
        m_actualVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_actualVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_actualVarLabel, wxSizerFlags{}.CenterVertical());

        auto* targetLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Target:"));
        targetLabel->SetFont(targetLabel->GetFont().Bold());
        varGrid->Add(targetLabel, wxSizerFlags{}.CenterVertical());
        m_targetVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_targetVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_targetVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // value display options
        auto* valueSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        valueSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Value format:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString valueFormatChoices;
        valueFormatChoices.Add(_(L"Value"));
        valueFormatChoices.Add(_(L"Percentage"));
        valueSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     valueFormatChoices, 0,
                                     wxGenericValidator{ &m_valueFormatIndex }));
        optionsSizer->Add(valueSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show value callouts"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showValueCallouts }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show range labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showRangeLabels }),
                          wxSizerFlags{}.Border());

        // range color scheme
        auto* colorSchemeBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Range Colors"));

        wxArrayString colorSchemeChoices;
        colorSchemeChoices.Add(_(L"Met/did not meet goal"));
        colorSchemeChoices.Add(_(L"Two-tone gradient"));
        colorSchemeBox->Add(new wxChoice(colorSchemeBox->GetStaticBox(), wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize, colorSchemeChoices, 0,
                                         wxGenericValidator{ &m_rangeColorSchemeIndex }),
                            wxSizerFlags{}.Border());

        auto* colorGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        colorGrid->Add(
            new wxStaticText(colorSchemeBox->GetStaticBox(), wxID_ANY, _(L"Met goal color:")),
            wxSizerFlags{}.CenterVertical());
        m_metGoalColorPicker =
            new wxColourPickerCtrl(colorSchemeBox->GetStaticBox(), wxID_ANY,
                                   Colors::ColorBrewer::GetColor(Colors::Color::KellyGreen));
        colorGrid->Add(m_metGoalColorPicker);

        colorGrid->Add(new wxStaticText(colorSchemeBox->GetStaticBox(), wxID_ANY,
                                        _(L"Did not meet goal color:")),
                       wxSizerFlags{}.CenterVertical());
        m_didNotMeetGoalColorPicker =
            new wxColourPickerCtrl(colorSchemeBox->GetStaticBox(), wxID_ANY,
                                   Colors::ColorBrewer::GetColor(Colors::Color::FireEngineRed));
        colorGrid->Add(m_didNotMeetGoalColorPicker);

        colorGrid->Add(
            new wxStaticText(colorSchemeBox->GetStaticBox(), wxID_ANY, _(L"Range start color:")),
            wxSizerFlags{}.CenterVertical());
        m_rangeStartColorPicker = new wxColourPickerCtrl(colorSchemeBox->GetStaticBox(), wxID_ANY,
                                                         wxColour{ 217, 217, 217 });
        colorGrid->Add(m_rangeStartColorPicker);

        colorGrid->Add(
            new wxStaticText(colorSchemeBox->GetStaticBox(), wxID_ANY, _(L"Range end color:")),
            wxSizerFlags{}.CenterVertical());
        m_rangeEndColorPicker = new wxColourPickerCtrl(colorSchemeBox->GetStaticBox(), wxID_ANY,
                                                       wxColour{ 89, 89, 89 });
        colorGrid->Add(m_rangeEndColorPicker);

        colorSchemeBox->Add(colorGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(colorSchemeBox, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateRangesPage();

        // prefill with a sample three-band scale
        // (LoadFromGraph will overwrite these for edit mode)
        m_ranges = { { 50, _(L"Poor") }, { 80, _(L"Satisfactory") }, { 100, _(L"Good") } };
        RefreshRangesList();

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertBulletChartDlg::CreateRangesPage()
        {
        auto* rangesPage = new wxPanel(GetSideBarBook());
        auto* pageMainSizer = new wxBoxSizer(wxVERTICAL);
        rangesPage->SetSizer(pageMainSizer);
        GetSideBarBook()->AddPage(rangesPage, _(L"Ranges"), ID_RANGES_SECTION);

        auto* rangesBox = new wxStaticBoxSizer(wxVERTICAL, rangesPage, _(L"Ranges"));

        m_rangesList = new wxListView(rangesBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                      wxSize{ -1, FromDIP(200) }, wxLC_REPORT | wxLC_SINGLE_SEL);
        m_rangesList->InsertColumn(0, _(L"Label"), wxLIST_FORMAT_LEFT, FromDIP(150));
        m_rangesList->InsertColumn(1, _(L"End value"), wxLIST_FORMAT_RIGHT, FromDIP(80));
        rangesBox->Add(m_rangesList, wxSizerFlags{ 1 }.Expand().Border());

        auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* addBtn = new wxButton(rangesBox->GetStaticBox(), wxID_ANY, _(L"Add"));
        auto* editBtn = new wxButton(rangesBox->GetStaticBox(), wxID_ANY, _(L"Edit"));
        auto* removeBtn = new wxButton(rangesBox->GetStaticBox(), wxID_ANY, _(L"Remove"));
        auto* moveUpBtn = new wxButton(rangesBox->GetStaticBox(), wxID_ANY, wxString{ L"▲" });
        auto* moveDownBtn = new wxButton(rangesBox->GetStaticBox(), wxID_ANY, wxString{ L"▼" });
        btnSizer->Add(addBtn, wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        btnSizer->Add(editBtn, wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        btnSizer->Add(removeBtn, wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        btnSizer->Add(moveUpBtn, wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        btnSizer->Add(moveDownBtn);
        rangesBox->Add(btnSizer, wxSizerFlags{}.Border(wxLEFT));

        pageMainSizer->Add(rangesBox, wxSizerFlags{ 1 }.Expand().Border());

        addBtn->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&) { OnAddRange(); });
        editBtn->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&) { OnEditRange(); });
        removeBtn->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnRemoveRange(); });
        moveUpBtn->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnMoveRangeUp(); });
        moveDownBtn->Bind(wxEVT_BUTTON,
                          [this]([[maybe_unused]] wxCommandEvent&) { OnMoveRangeDown(); });

        m_rangesList->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                           [this]([[maybe_unused]] wxListEvent&) { OnEditRange(); });
        }

    //-------------------------------------------
    long InsertBulletChartDlg::GetSelectedRangeIndex() const
        {
        return m_rangesList->GetFirstSelected();
        }

    //-------------------------------------------
    void InsertBulletChartDlg::RefreshRangesList()
        {
        const wxWindowUpdateLocker noUpdates(m_rangesList);
        m_rangesList->DeleteAllItems();
        for (size_t i = 0; i < m_ranges.size(); ++i)
            {
            const long idx = m_rangesList->InsertItem(static_cast<long>(i), m_ranges[i].m_label);
            m_rangesList->SetItem(idx, 1, wxString::FromDouble(m_ranges[i].m_end, 2));
            }
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnAddRange()
        {
        EditRangeDlg dlg(this, Graphs::BulletChart::Range{ 0, wxString{} });
        if (dlg.ShowModal() == wxID_OK)
            {
            m_ranges.emplace_back(dlg.GetRange());
            RefreshRangesList();
            const long newIdx = static_cast<long>(m_ranges.size()) - 1;
            m_rangesList->Select(newIdx);
            m_rangesList->EnsureVisible(newIdx);
            }
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnEditRange()
        {
        const long sel = GetSelectedRangeIndex();
        if (sel == wxNOT_FOUND)
            {
            return;
            }

        EditRangeDlg dlg(this, m_ranges[sel]);
        if (dlg.ShowModal() == wxID_OK)
            {
            m_ranges[sel] = dlg.GetRange();
            RefreshRangesList();
            m_rangesList->Select(sel);
            }
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnRemoveRange()
        {
        const long sel = GetSelectedRangeIndex();
        if (sel == wxNOT_FOUND)
            {
            return;
            }

        m_ranges.erase(m_ranges.begin() + sel);
        RefreshRangesList();
        if (!m_ranges.empty())
            {
            const long newSel = std::min(sel, static_cast<long>(m_ranges.size()) - 1);
            m_rangesList->Select(newSel);
            }
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnMoveRangeUp()
        {
        const long sel = GetSelectedRangeIndex();
        if (sel <= 0)
            {
            return;
            }

        std::swap(m_ranges[sel], m_ranges[sel - 1]);
        RefreshRangesList();
        m_rangesList->Select(sel - 1);
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnMoveRangeDown()
        {
        const long sel = GetSelectedRangeIndex();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel + 1, m_ranges.size()))
            {
            return;
            }

        std::swap(m_ranges[sel], m_ranges[sel + 1]);
        RefreshRangesList();
        m_rangesList->Select(sel + 1);
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnDatasetChanged()
        {
        m_labelVariable.clear();
        m_actualVariable.clear();
        m_targetVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertBulletChartDlg::OnSelectVariables()
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
                  .Label(_(L"Actual"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_actualVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_actualVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Target"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_targetVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_targetVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto labelVars = dlg.GetSelectedVariables(0);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        const auto actualVars = dlg.GetSelectedVariables(1);
        m_actualVariable = actualVars.empty() ? wxString{} : actualVars.front();

        const auto targetVars = dlg.GetSelectedVariables(2);
        m_targetVariable = targetVars.empty() ? wxString{} : targetVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertBulletChartDlg::UpdateVariableLabels()
        {
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_actualVarLabel->SetLabel(m_actualVariable);
        m_targetVarLabel->SetLabel(m_targetVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertBulletChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertBulletChartDlg::GetSelectedDataset() const
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
    wxColour InsertBulletChartDlg::GetRangeStartColor() const
        {
        return (m_rangeStartColorPicker != nullptr) ? m_rangeStartColorPicker->GetColour() :
                                                      wxColour{ 217, 217, 217 };
        }

    //-------------------------------------------
    wxColour InsertBulletChartDlg::GetRangeEndColor() const
        {
        return (m_rangeEndColorPicker != nullptr) ? m_rangeEndColorPicker->GetColour() :
                                                    wxColour{ 89, 89, 89 };
        }

    //-------------------------------------------
    wxColour InsertBulletChartDlg::GetMetGoalColor() const
        {
        return (m_metGoalColorPicker != nullptr) ?
                   m_metGoalColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::KellyGreen);
        }

    //-------------------------------------------
    wxColour InsertBulletChartDlg::GetDidNotMeetGoalColor() const
        {
        return (m_didNotMeetGoalColorPicker != nullptr) ?
                   m_didNotMeetGoalColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::FireEngineRed);
        }

    //-------------------------------------------
    bool InsertBulletChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_labelVariable.empty() || m_actualVariable.empty() || m_targetVariable.empty())
            {
            wxMessageBox(_(L"Please select the label, actual, and target variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertBulletChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* bulletChart = dynamic_cast<const Graphs::BulletChart*>(&graph);
        if (bulletChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = bulletChart->GetPropertyTemplate(L"dataset");
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
        m_labelVariable = bulletChart->GetLabelColumnName();
        m_actualVariable = bulletChart->GetActualColumnName();
        m_targetVariable = bulletChart->GetTargetColumnName();
        UpdateVariableLabels();

        // value display options
        m_valueFormatIndex =
            (bulletChart->GetValueDisplayFormat() == Graphs::BulletChartValueFormat::Percentage) ?
                1 :
                0;
        m_showValueCallouts = bulletChart->IsShowingValueCallouts();
        m_showRangeLabels = bulletChart->IsShowingRangeLabels();

        // range colors
        m_rangeColorSchemeIndex =
            (bulletChart->GetRangeColorScheme() == Graphs::BulletChartRangeColorScheme::TwoTone) ?
                1 :
                0;
        if (m_metGoalColorPicker != nullptr)
            {
            m_metGoalColorPicker->SetColour(bulletChart->GetGoalSuccessColor());
            }
        if (m_didNotMeetGoalColorPicker != nullptr)
            {
            m_didNotMeetGoalColorPicker->SetColour(bulletChart->GetGoalFailureColor());
            }
        if (m_rangeStartColorPicker != nullptr)
            {
            m_rangeStartColorPicker->SetColour(bulletChart->GetRangeStartColor());
            }
        if (m_rangeEndColorPicker != nullptr)
            {
            m_rangeEndColorPicker->SetColour(bulletChart->GetRangeEndColor());
            }

        // ranges
        m_ranges = bulletChart->GetRanges();
        RefreshRangesList();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::BulletChart>
    InsertBulletChartDlg::BuildBulletChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::BulletChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetRangeColorScheme(GetRangeColorScheme());
        plot->SetRangeStartColor(GetRangeStartColor());
        plot->SetRangeEndColor(GetRangeEndColor());
        plot->SetGoalSuccessColor(GetMetGoalColor());
        plot->SetGoalFailureColor(GetDidNotMeetGoalColor());
        plot->SetValueDisplayFormat(GetValueDisplayFormat());
        plot->ShowValueCallouts(IsShowingValueCallouts());
        plot->ShowRangeLabels(IsShowingRangeLabels());

        if (!GetRanges().empty())
            {
            plot->SetRanges(GetRanges());
            }

        plot->SetData(GetSelectedDataset(), GetLabelVariable(), GetActualVariable(),
                      GetTargetVariable());

        if (oldGraph != nullptr)
            {
            // The scaling axis's brackets are rebuilt from the ranges on every
            // SetData()/SetRanges() call, so axis state is not restored here.

            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::BulletChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.label", GetLabelVariable(),
                oldChart != nullptr ? oldChart->GetLabelColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.actual", GetActualVariable(),
                oldChart != nullptr ? oldChart->GetActualColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.target", GetTargetVariable(),
                oldChart != nullptr ? oldChart->GetTargetColumnName() : wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.label", GetLabelVariable());
            plot->SetPropertyTemplate(L"variables.actual", GetActualVariable());
            plot->SetPropertyTemplate(L"variables.target", GetTargetVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
