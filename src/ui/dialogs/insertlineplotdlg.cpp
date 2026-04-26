///////////////////////////////////////////////////////////////////////////////
// Name:        insertlineplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertlineplotdlg.h"
#include "../../graphs/lineplot.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertLinePlotDlg::InsertLinePlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertLinePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Line Plot"), ID_OPTIONS_SECTION, true);

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

        auto* yLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Values:"));
        yLabel->SetFont(yLabel->GetFont().Bold());
        varGrid->Add(yLabel, wxSizerFlags{}.CenterVertical());
        m_yVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_yVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_yVarLabel, wxSizerFlags{}.CenterVertical());

        auto* xLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Series:"));
        xLabel->SetFont(xLabel->GetFont().Bold());
        varGrid->Add(xLabel, wxSizerFlags{}.CenterVertical());
        m_xVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_xVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_xVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // line options
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Auto spline"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_autoSpline)),
                          wxSizerFlags{}.Border());

        // showcasing
        auto* ghostBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Showcasing"));

        auto* ghostOpacitySizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        ghostOpacitySizer->Add(
            new wxStaticText(ghostBox->GetStaticBox(), wxID_ANY, _(L"Ghost opacity:")),
            wxSizerFlags{}.CenterVertical());
        auto* opacitySpin = new wxSpinCtrl(ghostBox->GetStaticBox(), wxID_ANY);
        opacitySpin->SetRange(0, 255);
        opacitySpin->SetValidator(wxGenericValidator(&m_ghostOpacity));
        ghostOpacitySizer->Add(opacitySpin);
        ghostBox->Add(ghostOpacitySizer, wxSizerFlags{}.Border());

        m_showcaseListBox = new wxEditableListBox(
            ghostBox->GetStaticBox(), wxID_ANY, _(L"Showcase lines:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(120) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        ghostBox->Add(m_showcaseListBox, wxSizerFlags{ 1 }.Expand().Border());
        optionsSizer->Add(ghostBox, wxSizerFlags{ 1 }.Expand().Border());

        // override New button for showcase lines
        m_showcaseListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const auto dataset = GetSelectedDataset();
                if (dataset == nullptr || m_groupVariable.empty())
                    {
                    wxMessageBox(
                        _(L"Select a grouping variable first to populate available lines."),
                        _(L"No Groups"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }

                wxArrayString groupChoices;
                const auto groupCol = dataset->GetCategoricalColumn(m_groupVariable);
                if (groupCol != dataset->GetCategoricalColumns().cend())
                    {
                    for (const auto& [id, label] : groupCol->GetStringTable())
                        {
                        if (!label.empty())
                            {
                            groupChoices.Add(label);
                            }
                        }
                    }

                if (groupChoices.empty())
                    {
                    return;
                    }

                wxSingleChoiceDialog dlg(this, _(L"Select line to showcase:"), _(L"Showcase Line"),
                                         groupChoices);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    const auto val = dlg.GetStringSelection();
                    if (std::find(m_showcaseLines.begin(), m_showcaseLines.end(), val) ==
                        m_showcaseLines.end())
                        {
                        m_showcaseLines.push_back(val);
                        wxArrayString strings;
                        for (const auto& showLine : m_showcaseLines)
                            {
                            strings.Add(showLine);
                            }
                        m_showcaseListBox->SetStrings(strings);
                        }
                    }
            });

        // override Edit button
        m_showcaseListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const auto dataset = GetSelectedDataset();
                auto* listCtrl = m_showcaseListBox->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseLines.size()) ||
                    dataset == nullptr || m_groupVariable.empty())
                    {
                    return;
                    }

                wxArrayString groupChoices;
                const auto groupCol = dataset->GetCategoricalColumn(m_groupVariable);
                if (groupCol != dataset->GetCategoricalColumns().cend())
                    {
                    for (const auto& [id, label] : groupCol->GetStringTable())
                        {
                        if (!label.empty())
                            {
                            groupChoices.Add(label);
                            }
                        }
                    }

                if (groupChoices.empty())
                    {
                    return;
                    }

                wxSingleChoiceDialog dlg(this, _(L"Select line to showcase:"), _(L"Showcase Line"),
                                         groupChoices);
                dlg.SetSelection(sel);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    m_showcaseLines[sel] = dlg.GetStringSelection();
                    wxArrayString strings;
                    for (const auto& s : m_showcaseLines)
                        {
                        strings.Add(s);
                        }
                    m_showcaseListBox->SetStrings(strings);
                    }
            });

        // override Delete button
        m_showcaseListBox->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const long sel = m_showcaseListBox->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseLines.size()))
                    {
                    return;
                    }
                m_showcaseLines.erase(m_showcaseLines.begin() + sel);
                wxArrayString strings;
                for (const auto& s : m_showcaseLines)
                    {
                    strings.Add(s);
                    }
                m_showcaseListBox->SetStrings(strings);
            });

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateAnnotationsPage();
        CreateAxisOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertLinePlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertLinePlotDlg::OnSelectVariables()
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
            const int sel = m_datasetChoice->GetSelection();
            if (sel != wxNOT_FOUND && std::cmp_less(sel, m_datasetNames.size()))
                {
                const auto it = importOpts.find(m_datasetNames[sel]);
                if (it != importOpts.cend())
                    {
                    columnInfo = it->second.m_columnPreviewInfo;
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
                  .Label(/* TRANSLATORS: y-axis variable for line plot. */ _(L"Values"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_yVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_yVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(/* TRANSLATORS: x-axis variable for line plot. */ _(L"Series"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_xVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_xVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Grouping"))
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

        const auto yVars = dlg.GetSelectedVariables(0);
        m_yVariable = yVars.empty() ? wxString{} : yVars.front();

        const auto xVars = dlg.GetSelectedVariables(1);
        m_xVariable = xVars.empty() ? wxString{} : xVars.front();

        const auto groupVars = dlg.GetSelectedVariables(2);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertLinePlotDlg::UpdateVariableLabels()
        {
        m_yVarLabel->SetLabel(m_yVariable);
        m_xVarLabel->SetLabel(m_xVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        // clear showcase if group changes
        m_showcaseLines.clear();
        if (m_showcaseListBox != nullptr)
            {
            m_showcaseListBox->SetStrings(wxArrayString{});
            }

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertLinePlotDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertLinePlotDlg::GetSelectedDataset() const
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
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    bool InsertLinePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_yVariable.empty() || m_xVariable.empty())
            {
            wxMessageBox(_(L"Please select the Y and X variables."), _(L"Variable Not Specified"),
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
    void InsertLinePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* linePlot = dynamic_cast<const Graphs::LinePlot*>(&graph);
        if (linePlot == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = linePlot->GetPropertyTemplate(L"dataset");
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

        // load the actual column names from the graph
        m_xVariable = linePlot->GetXColumnName();
        m_yVariable = linePlot->GetYColumnName();
        m_groupVariable = linePlot->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // line-specific options
        m_autoSpline = linePlot->IsAutoSplining();
        m_ghostOpacity = linePlot->GetGhostOpacity();
        m_showcaseLines = linePlot->GetShowcasedLines();
        if (m_showcaseListBox != nullptr)
            {
            wxArrayString strings;
            for (const auto& showLine : m_showcaseLines)
                {
                strings.Add(showLine);
                }
            m_showcaseListBox->SetStrings(strings);
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
