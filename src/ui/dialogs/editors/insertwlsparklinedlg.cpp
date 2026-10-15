///////////////////////////////////////////////////////////////////////////////
// Name:        insertwlsparklinedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertwlsparklinedlg.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/win_loss_sparkline.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertWLSparklineDlg::InsertWLSparklineDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(
              canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
              static_cast<GraphDlgOptions>(GraphDlgIncludeMost & ~GraphDlgIncludeColorScheme))
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    std::vector<DataGuideParameter> InsertWLSparklineDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        const std::vector<Data::Dataset::ColumnImportType> categoricalTypes{
            Data::Dataset::ColumnImportType::String, Data::Dataset::ColumnImportType::Discrete,
            Data::Dataset::ColumnImportType::DichotomousString,
            Data::Dataset::ColumnImportType::DichotomousDiscrete
        };
        return { { .m_name = _(L"Season"),
                   .m_types = categoricalTypes,
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The label of the row that each game is placed in, such as "
                                      "the season or team."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Won/Lost"),
                   .m_types = categoricalTypes,
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The outcome of each game."),
                   .m_values = { { L"0", _(L"Lost") }, { L"1", _(L"Won") }, { L"2", _(L"Tied") } },
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Shutout"),
                   .m_types = categoricalTypes,
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"Whether the game was a shutout."),
                   .m_values = { { L"0", _(L"Not a shutout") }, { L"1", _(L"Shutout") } },
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Home game"),
                   .m_types = categoricalTypes,
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"Whether the game was played at home."),
                   .m_values = { { L"0", _(L"Away game") }, { L"1", _(L"Home game") } },
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} },
                 { .m_name = _(L"Postseason"),
                   .m_types = categoricalTypes,
                   .m_requirement = DataGuideRequirement::Optional,
                   .m_description = _(L"Whether the game was a postseason game."),
                   .m_values = { { L"0", _(L"Regular season") }, { L"1", _(L"Postseason") } },
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertWLSparklineDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Win/Loss Sparkline");
        guide.m_iconName = L"images/sparkline.svg";
        guide.m_summary = _(L"Shows the outcome of every game in a season as a compact row of "
                            "marks, with home games, shutouts, and streaks easy to spot.");
        guide.m_requirements =
            _(L"One row per game, with the season it belongs to and codes for its outcome, "
              "whether it was a shutout, and whether it was a home game.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample games;
        games.m_title = _(L"Example: the first games of two seasons");
        games.m_columns = { { _(L"Season"), _(L"Season") },
                            { _(L"Result"), _(L"Won/Lost") },
                            { _(L"Shutout"), _(L"Shutout") },
                            { _(L"HomeGame"), _(L"Home game") } };
        games.m_rows = { { L"2022", L"1", L"1", L"1" }, { L"2022", L"0", L"0", L"1" },
                         { L"2022", L"1", L"0", L"1" }, { L"2022", L"1", L"0", L"0" },
                         { L"2023", L"1", L"0", L"1" }, { L"2023", L"0", L"0", L"1" },
                         { L"2023", L"1", L"1", L"0" }, { L"2023", L"2", L"0", L"0" } };
        guide.m_examples = { std::move(games) };

        guide.m_missingData = { _(
            L"A blank cell marks a game that was canceled or a scrimmage. It is skipped when "
            "finding streaks and other statistics.") };
        guide.m_notes = { _(L"Games are placed in the order that they appear in the dataset.") };
        return guide;
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Win/Loss Sparkline"), ID_OPTIONS_SECTION, true);

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

        auto* seasonLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Season:"));
        seasonLabel->SetFont(seasonLabel->GetFont().Bold());
        varGrid->Add(seasonLabel, wxSizerFlags{}.CenterVertical());
        m_seasonVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_seasonVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_seasonVarLabel, wxSizerFlags{}.CenterVertical());

        auto* wonLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Won/Lost:"));
        wonLabel->SetFont(wonLabel->GetFont().Bold());
        varGrid->Add(wonLabel, wxSizerFlags{}.CenterVertical());
        m_wonVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_wonVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_wonVarLabel, wxSizerFlags{}.CenterVertical());

        auto* shutoutLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Shutout:"));
        shutoutLabel->SetFont(shutoutLabel->GetFont().Bold());
        varGrid->Add(shutoutLabel, wxSizerFlags{}.CenterVertical());
        m_shutoutVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_shutoutVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_shutoutVarLabel, wxSizerFlags{}.CenterVertical());

        auto* homeLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Home game:"));
        homeLabel->SetFont(homeLabel->GetFont().Bold());
        varGrid->Add(homeLabel, wxSizerFlags{}.CenterVertical());
        m_homeGameVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_homeGameVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_homeGameVarLabel, wxSizerFlags{}.CenterVertical());

        auto* postLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Postseason:"));
        postLabel->SetFont(postLabel->GetFont().Bold());
        varGrid->Add(postLabel, wxSizerFlags{}.CenterVertical());
        m_postseasonVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_postseasonVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_postseasonVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // highlight best records
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Highlight best records"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_highlightBestRecords }),
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
    void InsertWLSparklineDlg::OnDatasetChanged()
        {
        m_seasonVariable.clear();
        m_wonVariable.clear();
        m_shutoutVariable.clear();
        m_homeGameVariable.clear();
        m_postseasonVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::OnSelectVariables()
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
            { MakeVariableList(parameters[0], m_seasonVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_seasonVariable }),
              MakeVariableList(parameters[1], m_wonVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_wonVariable }),
              MakeVariableList(parameters[2], m_shutoutVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_shutoutVariable }),
              MakeVariableList(parameters[3], m_homeGameVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_homeGameVariable }),
              MakeVariableList(parameters[4], m_postseasonVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_postseasonVariable }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto seasonVars = dlg.GetSelectedVariables(0);
        m_seasonVariable = seasonVars.empty() ? wxString{} : seasonVars.front();

        const auto wonVars = dlg.GetSelectedVariables(1);
        m_wonVariable = wonVars.empty() ? wxString{} : wonVars.front();

        const auto shutoutVars = dlg.GetSelectedVariables(2);
        m_shutoutVariable = shutoutVars.empty() ? wxString{} : shutoutVars.front();

        const auto homeVars = dlg.GetSelectedVariables(3);
        m_homeGameVariable = homeVars.empty() ? wxString{} : homeVars.front();

        const auto postVars = dlg.GetSelectedVariables(4);
        m_postseasonVariable = postVars.empty() ? wxString{} : postVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::UpdateVariableLabels()
        {
        m_seasonVarLabel->SetLabel(m_seasonVariable);
        m_wonVarLabel->SetLabel(m_wonVariable);
        m_shutoutVarLabel->SetLabel(m_shutoutVariable);
        m_homeGameVarLabel->SetLabel(m_homeGameVariable);
        m_postseasonVarLabel->SetLabel(m_postseasonVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertWLSparklineDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_seasonVariable.empty() || m_wonVariable.empty() || m_shutoutVariable.empty() ||
            m_homeGameVariable.empty())
            {
            wxMessageBox(
                _(L"Please select the season, won/lost, shutout, and home game variables."),
                _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertWLSparklineDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* sparkline = dynamic_cast<const Graphs::WinLossSparkline*>(&graph);
        if (sparkline == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(sparkline->GetPropertyTemplate(L"dataset"));

        // load column names from the graph
        m_seasonVariable = sparkline->GetSeasonColumnName();
        m_wonVariable = sparkline->GetWonColumnName();
        m_shutoutVariable = sparkline->GetShutoutColumnName();
        m_homeGameVariable = sparkline->GetHomeGameColumnName();
        m_postseasonVariable = sparkline->GetPostseasonColumnName();
        UpdateVariableLabels();

        m_highlightBestRecords = sparkline->IsHighlightingBestRecords();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::WinLossSparkline>
    InsertWLSparklineDlg::BuildWinLossSparkline(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::WinLossSparkline>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);
        plot->HighlightBestRecords(GetHighlightBestRecords());

        const std::optional<wxString> postseasonCol =
            GetPostseasonVariable().empty() ? std::nullopt :
                                              std::optional<wxString>(GetPostseasonVariable());
        plot->SetData(GetSelectedDataset(), GetSeasonVariable(), GetWonVariable(),
                      GetShutoutVariable(), GetHomeGameVariable(), postseasonCol);

        if (oldGraph != nullptr)
            {
            ApplyAxisOverrides(*plot);

            const auto* oldWL = dynamic_cast<const Graphs::WinLossSparkline*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.season", GetSeasonVariable(),
                oldWL != nullptr ? oldWL->GetSeasonColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.won", GetWonVariable(),
                                               oldWL != nullptr ? oldWL->GetWonColumnName() :
                                                                  wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.shutout", GetShutoutVariable(),
                oldWL != nullptr ? oldWL->GetShutoutColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.home-game", GetHomeGameVariable(),
                oldWL != nullptr ? oldWL->GetHomeGameColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.postseason", GetPostseasonVariable(),
                oldWL != nullptr ? oldWL->GetPostseasonColumnName() : wxString{});
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.season", GetSeasonVariable());
            plot->SetPropertyTemplate(L"variables.won", GetWonVariable());
            plot->SetPropertyTemplate(L"variables.shutout", GetShutoutVariable());
            plot->SetPropertyTemplate(L"variables.home-game", GetHomeGameVariable());
            if (!GetPostseasonVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.postseason", GetPostseasonVariable());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
