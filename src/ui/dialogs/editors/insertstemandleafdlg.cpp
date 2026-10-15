///////////////////////////////////////////////////////////////////////////////
// Name:        insertstemandleafdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertstemandleafdlg.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/stemandleafplot.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertStemAndLeafDlg::InsertStemAndLeafDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    std::vector<DataGuideParameter> InsertStemAndLeafDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return { { .m_name = _(L"Continuous"),
                   .m_types = { Data::Dataset::ColumnImportType::Numeric },
                   .m_requirement = DataGuideRequirement::Required,
                   .m_description = _(L"The values to display. Each one is split into a stem and a "
                                      "leaf."),
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
                   .m_description = _(L"Compares two groups side by side on a shared stem. It must "
                                      "have exactly two unique values."),
                   .m_values = {},
                   .m_multiple = false,
                   .m_condition = wxString{},
                   .m_group = wxString{} } };
        }

    //-------------------------------------------
    DataGuide InsertStemAndLeafDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Stem-and-Leaf Plot");
        guide.m_iconName = L"images/stem-leaf.svg";
        guide.m_summary =
            _(L"Shows the distribution of a measurement while keeping every individual "
              "value, with the option of comparing two groups back to back.");
        guide.m_requirements =
            _(L"One row per observation, holding the value and, optionally, which of two "
              "groups the observation belongs to.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample scores;
        scores.m_title = _(L"Example: exam scores for two sections");
        scores.m_columns = { { _(L"Score"), _(L"Continuous") }, { _(L"Section"), _(L"Grouping") } };
        scores.m_rows = { { L"68", L"A" }, { L"75", L"A" }, { L"84", L"A" }, { L"91", L"A" },
                          { L"60", L"B" }, { L"71", L"B" }, { L"79", L"B" }, { L"88", L"B" } };
        guide.m_examples = { std::move(scores) };

        guide.m_missingData = { _(L"A missing value is skipped."),
                                _(L"A missing group is shown with an empty label.") };
        return guide;
        }

    //-------------------------------------------
    void InsertStemAndLeafDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Stem-and-Leaf"), ID_OPTIONS_SECTION, true);

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

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertStemAndLeafDlg::OnDatasetChanged()
        {
        m_continuousVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertStemAndLeafDlg::OnSelectVariables()
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
            { MakeVariableList(parameters[0], m_continuousVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_continuousVariable }),
              MakeVariableList(parameters[1], m_groupVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_groupVariable }) });

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
    void InsertStemAndLeafDlg::UpdateVariableLabels()
        {
        m_continuousVarLabel->SetLabel(m_continuousVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertStemAndLeafDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_continuousVariable.empty())
            {
            wxMessageBox(_(L"Please select the continuous variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertStemAndLeafDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* stemLeaf = dynamic_cast<const Graphs::StemAndLeafPlot*>(&graph);
        if (stemLeaf == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(stemLeaf->GetPropertyTemplate(L"dataset"));

        // load column names from the graph
        m_continuousVariable = stemLeaf->GetContinuousColumnName();
        m_groupVariable = stemLeaf->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::StemAndLeafPlot>
    InsertStemAndLeafDlg::BuildStemAndLeafPlot(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::StemAndLeafPlot>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        const std::optional<wxString> groupCol =
            GetGroupVariable().empty() ? std::nullopt : std::optional<wxString>(GetGroupVariable());
        plot->SetData(GetSelectedDataset(), GetContinuousVariable(), groupCol);

        if (oldGraph != nullptr)
            {
            ApplyAxisOverrides(*plot);

            const auto* oldSL = dynamic_cast<const Graphs::StemAndLeafPlot*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.continuous", GetContinuousVariable(),
                oldSL != nullptr ? oldSL->GetContinuousColumnName() : wxString{});
            const auto oldGroupName =
                (oldSL != nullptr) ? oldSL->GetGroupColumnName().value_or(wxString{}) : wxString{};
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
