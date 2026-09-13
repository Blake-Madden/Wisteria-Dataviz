///////////////////////////////////////////////////////////////////////////////
// Name:        insert_nightingale_rose_chart_dlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insert_nightingale_rose_chart_dlg.h"
#include "../../app/wisteriaview.h"
#include "../variableselectdlg.h"
#include <iterator>
#include <utility>
#include <wx/combobox.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertNightingaleRoseChartDlg::InsertNightingaleRoseChartDlg(
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
    void InsertNightingaleRoseChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Nightingale Rose Chart"), ID_OPTIONS_SECTION,
                                  true);

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

        auto* categoryLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Category:"));
        categoryLabel->SetFont(categoryLabel->GetFont().Bold());
        varGrid->Add(categoryLabel, wxSizerFlags{}.CenterVertical());
        m_categoryVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_categoryVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_categoryVarLabel, wxSizerFlags{}.CenterVertical());

        auto* aggregateLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Aggregate:"));
        aggregateLabel->SetFont(aggregateLabel->GetFont().Bold());
        varGrid->Add(aggregateLabel, wxSizerFlags{}.CenterVertical());
        m_aggregateVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_aggregateVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_aggregateVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Series:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // rose layout options
        auto* roseSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        roseSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Radial scaling:")),
                       wxSizerFlags{}.CenterVertical());
        wxArrayString radialScalingChoices;
        radialScalingChoices.Add(_(L"Area proportional"));
        radialScalingChoices.Add(_(L"Radius proportional"));
        roseSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    radialScalingChoices, 0,
                                    wxGenericValidator{ &m_radialScalingSelection }));

        roseSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Series display:")),
                       wxSizerFlags{}.CenterVertical());
        wxArrayString seriesDisplayChoices;
        seriesDisplayChoices.Add(_(L"Overlaid"));
        seriesDisplayChoices.Add(_(L"Stacked"));
        roseSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    seriesDisplayChoices, 0,
                                    wxGenericValidator{ &m_seriesDisplaySelection }));

        roseSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Start angle (degrees):")),
                       wxSizerFlags{}.CenterVertical());
            {
            m_startAngleSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY);
            m_startAngleSpin->SetRange(0, 359.9);
            m_startAngleSpin->SetDigits(1);
            m_startAngleSpin->SetIncrement(5);
            m_startAngleSpin->SetValue(90.0);
            roseSizer->Add(m_startAngleSpin);
            }

        optionsSizer->Add(roseSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show category labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showLabels }),
                          wxSizerFlags{}.Border());

        // ghosting
        auto* ghostBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Ghosting"));

        auto* ghostOpacitySizer = new wxBoxSizer(wxHORIZONTAL);
        ghostOpacitySizer->Add(
            new wxStaticText(ghostBox->GetStaticBox(), wxID_ANY, _(L"Ghost opacity:")),
            wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
            {
            auto* ghostOpacitySpin = new wxSpinCtrl(ghostBox->GetStaticBox(), wxID_ANY);
            ghostOpacitySpin->SetRange(0, 255);
            ghostOpacitySpin->SetValidator(wxGenericValidator{ &m_ghostOpacity });
            ghostOpacitySizer->Add(ghostOpacitySpin);
            }
        ghostBox->Add(ghostOpacitySizer, wxSizerFlags{}.Border());

        m_ghostedWedgesList = new wxEditableListBox(
            ghostBox->GetStaticBox(), wxID_ANY, _(L"Wedges to ghost (series / category):"),
            wxDefaultPosition, wxDefaultSize, wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_NO_REORDER);
        ghostBox->Add(m_ghostedWedgesList, wxSizerFlags{ 1 }.Expand().Border());

        // rows are structured pairs edited through EditGhostOptions(), so block in-place editing
        m_ghostedWedgesList->GetListCtrl()->Bind(wxEVT_LIST_BEGIN_LABEL_EDIT,
                                                 []([[maybe_unused]]
                                                    wxListEvent& labelEditEvent)
                                                 { labelEditEvent.Veto(); });

        m_ghostedWedgesList->GetNewButton()->Bind(wxEVT_BUTTON,
                                                  [this]([[maybe_unused]] wxCommandEvent&)
                                                  {
                                                      wxString gpLabel;
                                                      wxString catLabel;
                                                      if (EditGhostOptions(gpLabel, catLabel))
                                                          {
                                                          m_ghostedWedges.emplace_back(gpLabel,
                                                                                       catLabel);
                                                          RefreshGhostedWedgesList();
                                                          }
                                                  });
        m_ghostedWedgesList->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] wxCommandEvent&)
            {
                auto* listCtrl = m_ghostedWedgesList->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_ghostedWedges.size()))
                    {
                    return;
                    }
                wxString gpLabel = m_ghostedWedges[sel].first;
                wxString catLabel = m_ghostedWedges[sel].second;
                if (EditGhostOptions(gpLabel, catLabel))
                    {
                    m_ghostedWedges[sel] = { gpLabel, catLabel };
                    RefreshGhostedWedgesList();
                    }
            });
        m_ghostedWedgesList->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] wxCommandEvent&)
            {
                const long sel = m_ghostedWedgesList->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_ghostedWedges.size()))
                    {
                    return;
                    }
                m_ghostedWedges.erase(std::next(m_ghostedWedges.cbegin(), sel));
                RefreshGhostedWedgesList();
            });

        optionsSizer->Add(ghostBox, wxSizerFlags{ 1 }.Expand().Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertNightingaleRoseChartDlg::OnDatasetChanged()
        {
        m_categoryVariable.clear();
        m_aggregateVariable.clear();
        m_groupVariable.clear();
        m_ghostedWedges.clear();
        RefreshGhostedWedgesList();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertNightingaleRoseChartDlg::OnSelectVariables()
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
                  .Label(_(L"Category"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_categoryVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_categoryVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Aggregate"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_aggregateVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_aggregateVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Series"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto categoryVars = dlg.GetSelectedVariables(0);
        m_categoryVariable = categoryVars.empty() ? wxString{} : categoryVars.front();

        const auto aggregateVars = dlg.GetSelectedVariables(1);
        m_aggregateVariable = aggregateVars.empty() ? wxString{} : aggregateVars.front();

        const auto groupVars = dlg.GetSelectedVariables(2);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertNightingaleRoseChartDlg::UpdateVariableLabels()
        {
        m_categoryVarLabel->SetLabel(m_categoryVariable);
        m_aggregateVarLabel->SetLabel(m_aggregateVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertNightingaleRoseChartDlg::RefreshGhostedWedgesList()
        {
        if (m_ghostedWedgesList == nullptr)
            {
            return;
            }
        wxArrayString strings;
        for (const auto& [groupLabel, categoryLabel] : m_ghostedWedges)
            {
            strings.Add(categoryLabel.empty() ?
                            wxString::Format(_(L"%s (all slices)"), groupLabel) :
                            wxString::Format(L"%s / %s", groupLabel, categoryLabel));
            }
        m_ghostedWedgesList->SetStrings(strings);
        }

    //-------------------------------------------
    bool InsertNightingaleRoseChartDlg::EditGhostOptions(wxString& groupLabel,
                                                         wxString& categoryLabel)
        {
        // offer the distinct labels of the series and category columns as choices
        wxArrayString groupChoices;
        wxArrayString categoryChoices;
        if (const auto dataset = GetSelectedDataset(); dataset != nullptr)
            {
            const auto addLabels = [&dataset](const wxString& colName, wxArrayString& out)
            {
                if (colName.empty())
                    {
                    return;
                    }
                const auto col = dataset->GetCategoricalColumn(colName);
                if (col != dataset->GetCategoricalColumns().cend())
                    {
                    for (const auto& strTableEntry : col->GetStringTable())
                        {
                        out.Add(strTableEntry.second);
                        }
                    }
            };
            addLabels(m_groupVariable, groupChoices);
            addLabels(m_categoryVariable, categoryChoices);
            }

        wxDialog dlg(this, wxID_ANY, _(L"Ghosted Wedge"));
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        auto* grid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Series:")), wxSizerFlags{}.CenterVertical());
        auto* groupCombo = new wxComboBox(&dlg, wxID_ANY, groupLabel, wxDefaultPosition,
                                          wxDefaultSize, groupChoices);
        grid->Add(groupCombo, wxSizerFlags{}.Expand());
        grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Category:")),
                  wxSizerFlags{}.CenterVertical());
        auto* categoryCombo = new wxComboBox(&dlg, wxID_ANY, categoryLabel, wxDefaultPosition,
                                             wxDefaultSize, categoryChoices);
        grid->Add(categoryCombo, wxSizerFlags{}.Expand());
        grid->AddGrowableCol(1);
        sizer->Add(grid, wxSizerFlags{ 1 }.Expand().Border());

        sizer->Add(
            new wxStaticText(&dlg, wxID_ANY,
                             _(L"Leave the category blank to ghost the series in every slice.")),
            wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        sizer->Add(dlg.CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                   wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(sizer);
        dlg.Centre();

        for (;;)
            {
            if (dlg.ShowModal() != wxID_OK)
                {
                return false;
                }
            if (wxString{ groupCombo->GetValue() }.Trim().Trim(false).empty())
                {
                wxMessageBox(_(L"Please enter a series label."), _(L"Series Required"),
                             wxOK | wxICON_WARNING, &dlg);
                continue;
                }
            groupLabel = groupCombo->GetValue();
            categoryLabel = categoryCombo->GetValue();
            return true;
            }
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertNightingaleRoseChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertNightingaleRoseChartDlg::GetSelectedDataset() const
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
    Graphs::NightingaleRoseChart::RadialScaling
    InsertNightingaleRoseChartDlg::GetRadialScaling() const noexcept
        {
        return (m_radialScalingSelection == 1) ?
                   Graphs::NightingaleRoseChart::RadialScaling::RadiusProportional :
                   Graphs::NightingaleRoseChart::RadialScaling::AreaProportional;
        }

    //-------------------------------------------
    Graphs::NightingaleRoseChart::SeriesDisplay
    InsertNightingaleRoseChartDlg::GetSeriesDisplay() const noexcept
        {
        return (m_seriesDisplaySelection == 1) ?
                   Graphs::NightingaleRoseChart::SeriesDisplay::Stacked :
                   Graphs::NightingaleRoseChart::SeriesDisplay::Overlaid;
        }

    //-------------------------------------------
    double InsertNightingaleRoseChartDlg::GetStartAngle() const
        {
        return (m_startAngleSpin != nullptr) ? m_startAngleSpin->GetValue() : 90.0;
        }

    //-------------------------------------------
    bool InsertNightingaleRoseChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_categoryVariable.empty())
            {
            wxMessageBox(_(L"Please select the category variable."), _(L"Variable Not Specified"),
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
    void InsertNightingaleRoseChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* roseChart = dynamic_cast<const Graphs::NightingaleRoseChart*>(&graph);
        if (roseChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = roseChart->GetPropertyTemplate(L"dataset");
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
        m_categoryVariable = roseChart->GetCategoryColumnName();
        m_aggregateVariable = roseChart->GetAggregateColumnName();
        m_groupVariable = roseChart->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // rose layout options
        m_radialScalingSelection =
            (roseChart->GetRadialScaling() ==
             Graphs::NightingaleRoseChart::RadialScaling::RadiusProportional) ?
                1 :
                0;
        m_seriesDisplaySelection = (roseChart->GetSeriesDisplay() ==
                                    Graphs::NightingaleRoseChart::SeriesDisplay::Stacked) ?
                                       1 :
                                       0;
        m_showLabels = roseChart->IsShowingLabels();

        m_startAngleSpin->SetValue(roseChart->GetStartAngle());

        m_ghostOpacity = static_cast<int>(roseChart->GetGhostOpacity());
        m_ghostedWedges.assign(roseChart->GetGhostedWedges().cbegin(),
                               roseChart->GetGhostedWedges().cend());
        RefreshGhostedWedgesList();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::NightingaleRoseChart>
    InsertNightingaleRoseChartDlg::BuildNightingaleRoseChart(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::NightingaleRoseChart>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        const std::optional<wxString> aggregateCol =
            GetAggregateVariable().empty() ? std::nullopt :
                                             std::optional<wxString>(GetAggregateVariable());
        const std::optional<wxString> groupCol =
            GetGroupVariable().empty() ? std::nullopt : std::optional<wxString>(GetGroupVariable());
        plot->SetData(GetSelectedDataset(), aggregateCol, GetCategoryVariable(), groupCol);

        if (oldGraph != nullptr)
            {
            ApplyAxisOverrides(*plot);
            }

        plot->SetRadialScaling(GetRadialScaling());
        plot->SetSeriesDisplay(GetSeriesDisplay());
        plot->SetStartAngle(GetStartAngle());
        plot->ShowLabels(IsShowingLabels());
        plot->SetGhostOpacity(GetGhostOpacity());
        for (const auto& [ghostGroupLabel, ghostCategoryLabel] : GetGhostedWedges())
            {
            plot->GhostWedge(ghostGroupLabel, ghostCategoryLabel);
            }

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            const auto* oldChart = dynamic_cast<const Graphs::NightingaleRoseChart*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.category", GetCategoryVariable(),
                oldChart != nullptr ? oldChart->GetCategoryColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.aggregate", GetAggregateVariable(),
                oldChart != nullptr ? oldChart->GetAggregateColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.group", GetGroupVariable(),
                oldChart != nullptr ? oldChart->GetGroupColumnName().value_or(wxString{}) :
                                      wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.category", GetCategoryVariable());
            if (!GetAggregateVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.aggregate", GetAggregateVariable());
                }
            if (!GetGroupVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.group", GetGroupVariable());
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
