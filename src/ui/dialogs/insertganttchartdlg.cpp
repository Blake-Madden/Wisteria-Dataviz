///////////////////////////////////////////////////////////////////////////////
// Name:        insertganttchartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertganttchartdlg.h"
#include "../../graphs/ganttchart.h"
#include "variableselectdlg.h"
#include <algorithm>
#include <set>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertGanttChartDlg::InsertGanttChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertGanttChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();
        CreateAxisOptionsPage();
        CreateAnnotationsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Gantt Chart"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

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

        auto* taskLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Task:"));
        taskLabel->SetFont(taskLabel->GetFont().Bold());
        varGrid->Add(taskLabel, wxSizerFlags{}.CenterVertical());
        m_taskVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_taskVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_taskVarLabel, wxSizerFlags{}.CenterVertical());

        auto* startLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Start date:"));
        startLabel->SetFont(startLabel->GetFont().Bold());
        varGrid->Add(startLabel, wxSizerFlags{}.CenterVertical());
        m_startDateVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_startDateVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_startDateVarLabel, wxSizerFlags{}.CenterVertical());

        auto* endLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"End date:"));
        endLabel->SetFont(endLabel->GetFont().Bold());
        varGrid->Add(endLabel, wxSizerFlags{}.CenterVertical());
        m_endDateVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_endDateVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_endDateVarLabel, wxSizerFlags{}.CenterVertical());

        auto* resourceLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Resource:"));
        resourceLabel->SetFont(resourceLabel->GetFont().Bold());
        varGrid->Add(resourceLabel, wxSizerFlags{}.CenterVertical());
        m_resourceVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_resourceVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_resourceVarLabel, wxSizerFlags{}.CenterVertical());

        auto* descLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Description:"));
        descLabel->SetFont(descLabel->GetFont().Bold());
        varGrid->Add(descLabel, wxSizerFlags{}.CenterVertical());
        m_descriptionVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_descriptionVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_descriptionVarLabel, wxSizerFlags{}.CenterVertical());

        auto* completionLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Completion:"));
        completionLabel->SetFont(completionLabel->GetFont().Bold());
        varGrid->Add(completionLabel, wxSizerFlags{}.CenterVertical());
        m_completionVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_completionVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_completionVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // date interval
        auto* optGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        optGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Date interval:")),
                     wxSizerFlags{}.CenterVertical());
            {
            auto* choice = new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                        nullptr, 0, wxGenericValidator(&m_dateInterval));
            choice->Append(_(L"Daily"));
            choice->Append(_(L"Fiscal quarterly"));
            choice->Append(_(L"Monthly"));
            choice->Append(_(L"Weekly"));
            optGrid->Add(choice);
            }

        // fiscal year type
        optGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Fiscal year type:")),
                     wxSizerFlags{}.CenterVertical());
            {
            auto* choice = new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                        nullptr, 0, wxGenericValidator(&m_fyType));
            choice->Append(_(L"Education"));
            choice->Append(_(L"US Business"));
            optGrid->Add(choice);
            }

        // task label display
        optGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Task label display:")),
                     wxSizerFlags{}.CenterVertical());
            {
            auto* choice = new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                        nullptr, 0, wxGenericValidator(&m_taskLabelDisplay));
            choice->Append(_(L"Resource"));
            choice->Append(_(L"Description"));
            choice->Append(_(L"Resource and description"));
            choice->Append(_(L"Days"));
            choice->Append(_(L"Resource and days"));
            choice->Append(_(L"Description and days"));
            choice->Append(_(L"Resource, description, and days"));
            choice->Append(_(L"No display"));
            optGrid->Add(choice);
            }

        optionsSizer->Add(optGrid, wxSizerFlags{}.Border());

        // bar shapes
        auto* shapesBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Bar Shapes"));

        m_shapeAllRadio =
            new wxRadioButton(shapesBox->GetStaticBox(), wxID_ANY, _(L"Same shape for all bars:"),
                              wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        m_shapeAllRadio->SetValue(true);
        shapesBox->Add(m_shapeAllRadio, wxSizerFlags{}.Border());

        wxArrayString barShapeChoices;
        barShapeChoices.Add(_(L"Rectangle"));
        barShapeChoices.Add(_(L"Arrow"));
        barShapeChoices.Add(_(L"Reverse Arrow"));
        m_shapeAllChoice =
            new wxChoice(shapesBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                         barShapeChoices, 0, wxGenericValidator(&m_barShapeAllIndex));
        shapesBox->Add(m_shapeAllChoice, wxSizerFlags{}.Border(wxLEFT | wxBOTTOM));

        m_shapePerBarRadio =
            new wxRadioButton(shapesBox->GetStaticBox(), wxID_ANY, _(L"Custom per-bar shapes:"));
        shapesBox->Add(m_shapePerBarRadio, wxSizerFlags{}.Border());

        m_shapePerBarListBox = new wxEditableListBox(
            shapesBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(150) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        m_shapePerBarListBox->Enable(false);
        shapesBox->Add(m_shapePerBarListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        optionsSizer->Add(shapesBox, wxSizerFlags{}.Expand().Border());

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // showcasing
        auto* ghostBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Showcasing"));

        auto* ghostOpacitySizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        ghostOpacitySizer->Add(
            new wxStaticText(ghostBox->GetStaticBox(), wxID_ANY, _(L"Ghost opacity:")),
            wxSizerFlags{}.CenterVertical());
        auto* opacitySpin = new wxSpinCtrl(ghostBox->GetStaticBox(), wxID_ANY);
        opacitySpin->SetRange(0, 255);
        opacitySpin->SetValidator(wxGenericValidator(&m_ghostOpacity));
        ghostOpacitySizer->Add(opacitySpin);
        ghostBox->Add(ghostOpacitySizer, wxSizerFlags{}.Border());

        m_showcaseListBox = new wxEditableListBox(
            ghostBox->GetStaticBox(), wxID_ANY, _(L"Showcase bars:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(120) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        ghostBox->Add(m_showcaseListBox, wxSizerFlags{ 1 }.Expand().Border());
        optionsSizer->Add(ghostBox, wxSizerFlags{ 1 }.Expand().Border());

        // Helper to collect available bar labels
        auto gatherBarLabels = [this]() -> wxArrayString
        {
            wxArrayString labels;
            for (const auto& label : m_taskLabels)
                {
                labels.Add(label);
                }
            return labels;
        };

        const auto promptForShape = [this](const wxArrayString& barChoices, const wxString& caption,
                                           wxString& inOutLabel,
                                           Graphs::BarChart::BarShape& inOutShape) -> bool
        {
            wxArrayString shapeChoices;
            shapeChoices.Add(_(L"Rectangle"));
            shapeChoices.Add(_(L"Arrow"));
            shapeChoices.Add(_(L"Reverse Arrow"));

            wxDialog dlg(this, wxID_ANY, caption, wxDefaultPosition, wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
            auto* sizer = new wxBoxSizer(wxVERTICAL);
            auto* grid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
            grid->AddGrowableCol(1, 1);

            grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Bar:")),
                      wxSizerFlags{}.CenterVertical());
            auto* barCtrl =
                new wxChoice(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, barChoices);
            if (!inOutLabel.empty())
                {
                barCtrl->SetStringSelection(inOutLabel);
                }
            if (barCtrl->GetSelection() == wxNOT_FOUND)
                {
                barCtrl->SetSelection(0);
                }
            grid->Add(barCtrl, wxSizerFlags{}.Expand());

            grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Shape:")),
                      wxSizerFlags{}.CenterVertical());
            auto* shapeCtrl =
                new wxChoice(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, shapeChoices);
            shapeCtrl->SetSelection(BarShapeToIndex(inOutShape));
            grid->Add(shapeCtrl, wxSizerFlags{}.Expand());

            sizer->Add(grid, wxSizerFlags{ 1 }.Expand().Border());
            sizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
            dlg.SetSizer(sizer);
            dlg.Fit();
            dlg.SetMinSize(dlg.GetSize());

            if (dlg.ShowModal() != wxID_OK)
                {
                return false;
                }
            const auto barSel = barCtrl->GetSelection();
            if (barSel == wxNOT_FOUND)
                {
                return false;
                }
            inOutLabel = barChoices[barSel];
            inOutShape = BarShapeFromIndex(shapeCtrl->GetSelection());
            return true;
        };

        // override New button for showcase bars
        m_showcaseListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this, gatherBarLabels]([[maybe_unused]]
                                    wxCommandEvent& event)
            {
                const auto choices = gatherBarLabels();
                if (choices.empty())
                    {
                    wxMessageBox(_(L"Select a task variable first to populate available bars."),
                                 _(L"No Tasks"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }

                wxSingleChoiceDialog dlg(this, _(L"Select bar to showcase:"), _(L"Showcase Bar"),
                                         choices);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    const auto val = dlg.GetStringSelection();
                    if (std::find(m_showcaseBars.begin(), m_showcaseBars.end(), val) ==
                        m_showcaseBars.end())
                        {
                        m_showcaseBars.push_back(val);
                        wxArrayString strings;
                        for (const auto& showBar : m_showcaseBars)
                            {
                            strings.Add(showBar);
                            }
                        m_showcaseListBox->SetStrings(strings);
                        }
                    }
            });

        // override Edit button
        m_showcaseListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this, gatherBarLabels]([[maybe_unused]]
                                    wxCommandEvent& event)
            {
                auto* listCtrl = m_showcaseListBox->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseBars.size()))
                    {
                    return;
                    }

                const auto choices = gatherBarLabels();
                if (choices.empty())
                    {
                    return;
                    }

                wxSingleChoiceDialog dlg(this, _(L"Select bar to showcase:"), _(L"Showcase Bar"),
                                         choices);
                dlg.SetSelection(sel);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    m_showcaseBars[sel] = dlg.GetStringSelection();
                    wxArrayString strings;
                    for (const auto& s : m_showcaseBars)
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
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseBars.size()))
                    {
                    return;
                    }
                m_showcaseBars.erase(m_showcaseBars.begin() + sel);
                wxArrayString strings;
                for (const auto& showBar : m_showcaseBars)
                    {
                    strings.Add(showBar);
                    }
                m_showcaseListBox->SetStrings(strings);
            });

        // override New button for per-bar shapes
        m_shapePerBarListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this, gatherBarLabels, promptForShape]([[maybe_unused]]
                                                    wxCommandEvent& event)
            {
                const auto barChoices = gatherBarLabels();
                if (barChoices.empty())
                    {
                    wxMessageBox(_(L"Select a task variable first to populate bar labels."),
                                 _(L"No Bars"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }
                wxString label;
                auto shape = Graphs::BarChart::BarShape::Arrow;
                if (!promptForShape(barChoices, _(L"Add Per-Bar Shape"), label, shape))
                    {
                    return;
                    }
                const auto it =
                    std::find_if(m_barShapes.begin(), m_barShapes.end(),
                                 [&label](const auto& entry) { return entry.first == label; });
                if (it != m_barShapes.end())
                    {
                    it->second = shape;
                    }
                else
                    {
                    m_barShapes.emplace_back(label, shape);
                    }
                SyncBarShapesToList();
            });

        // override Edit button for per-bar shapes
        m_shapePerBarListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this, gatherBarLabels, promptForShape]([[maybe_unused]]
                                                    wxCommandEvent& event)
            {
                auto* listCtrl = m_shapePerBarListBox->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_barShapes.size()))
                    {
                    return;
                    }
                const auto barChoices = gatherBarLabels();
                if (barChoices.empty())
                    {
                    return;
                    }
                auto& entry = m_barShapes[sel];
                wxString label = entry.first;
                auto shape = entry.second;
                if (!promptForShape(barChoices, _(L"Edit Per-Bar Shape"), label, shape))
                    {
                    return;
                    }
                entry.first = label;
                entry.second = shape;
                SyncBarShapesToList();
            });

        // override Delete button for per-bar shapes
        m_shapePerBarListBox->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const long sel = m_shapePerBarListBox->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_barShapes.size()))
                    {
                    return;
                    }
                m_barShapes.erase(m_barShapes.begin() + sel);
                SyncBarShapesToList();
            });

        // double-clicking a row triggers the Edit button
        m_shapePerBarListBox->GetListCtrl()->Bind(
            wxEVT_LIST_ITEM_ACTIVATED,
            [this]([[maybe_unused]]
                   wxListEvent& event)
            {
                auto* editBtn = m_shapePerBarListBox->GetEditButton();
                wxCommandEvent clickEvent(wxEVT_BUTTON, editBtn->GetId());
                clickEvent.SetEventObject(editBtn);
                editBtn->GetEventHandler()->ProcessEvent(clickEvent);
            });

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        m_shapeAllRadio->Bind(wxEVT_RADIOBUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                              { OnBarShapeModeChanged(); });
        m_shapePerBarRadio->Bind(wxEVT_RADIOBUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                 { OnBarShapeModeChanged(); });
        m_shapeAllChoice->Bind(wxEVT_CHOICE, [this]([[maybe_unused]] wxCommandEvent&)
                               { OnBarShapeAllChanged(); });
        }

    //-------------------------------------------
    void InsertGanttChartDlg::OnDatasetChanged()
        {
        m_taskVariable.clear();
        m_startDateVariable.clear();
        m_endDateVariable.clear();
        m_resourceVariable.clear();
        m_descriptionVariable.clear();
        m_completionVariable.clear();
        m_groupVariable.clear();
        m_barShapes.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertGanttChartDlg::OnSelectVariables()
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
                  .Label(_(L"Task"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_taskVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_taskVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Start date"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_startDateVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_startDateVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Date }),
              VLI{}
                  .Label(_(L"End date"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_endDateVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_endDateVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Date }),
              VLI{}
                  .Label(_(L"Resource"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_resourceVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_resourceVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Description"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_descriptionVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_descriptionVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Completion"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_completionVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_completionVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
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

        const auto taskVars = dlg.GetSelectedVariables(0);
        m_taskVariable = taskVars.empty() ? wxString{} : taskVars.front();

        const auto startVars = dlg.GetSelectedVariables(1);
        m_startDateVariable = startVars.empty() ? wxString{} : startVars.front();

        const auto endVars = dlg.GetSelectedVariables(2);
        m_endDateVariable = endVars.empty() ? wxString{} : endVars.front();

        const auto resVars = dlg.GetSelectedVariables(3);
        m_resourceVariable = resVars.empty() ? wxString{} : resVars.front();

        const auto descVars = dlg.GetSelectedVariables(4);
        m_descriptionVariable = descVars.empty() ? wxString{} : descVars.front();

        const auto compVars = dlg.GetSelectedVariables(5);
        m_completionVariable = compVars.empty() ? wxString{} : compVars.front();

        const auto groupVars = dlg.GetSelectedVariables(6);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertGanttChartDlg::UpdateVariableLabels()
        {
        m_taskVarLabel->SetLabel(m_taskVariable);
        m_startDateVarLabel->SetLabel(m_startDateVariable);
        m_endDateVarLabel->SetLabel(m_endDateVariable);
        m_resourceVarLabel->SetLabel(m_resourceVariable);
        m_descriptionVarLabel->SetLabel(m_descriptionVariable);
        m_completionVarLabel->SetLabel(m_completionVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        RefreshTaskLabels();
        SyncBarShapesToList();

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertGanttChartDlg::RefreshTaskLabels()
        {
        m_taskLabels.clear();
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr || m_taskVariable.empty())
            {
            return;
            }
        const auto catCol = dataset->GetCategoricalColumn(m_taskVariable);
        if (catCol == dataset->GetCategoricalColumns().cend())
            {
            return;
            }
        std::set<wxString, Data::wxStringLessNoCase> seen;
        for (size_t i = 0; i < dataset->GetRowCount(); ++i)
            {
            const auto label = catCol->GetValueAsLabel(i);
            if (seen.insert(label).second)
                {
                m_taskLabels.push_back(label);
                }
            }
        }

    //-------------------------------------------
    void InsertGanttChartDlg::SyncBarShapesToList()
        {
        if (m_shapePerBarListBox == nullptr)
            {
            return;
            }
        const auto shapeLabel = [](const Graphs::BarChart::BarShape shape) -> wxString
        {
            switch (shape)
                {
            case Graphs::BarChart::BarShape::Arrow:
                return _(L"Arrow");
            case Graphs::BarChart::BarShape::ReverseArrow:
                return _(L"Reverse Arrow");
            case Graphs::BarChart::BarShape::Rectangle:
                [[fallthrough]];
            default:
                return _(L"Rectangle");
                }
        };
        wxArrayString items;
        for (const auto& [label, shape] : m_barShapes)
            {
            items.Add(label + L": " + shapeLabel(shape));
            }
        m_shapePerBarListBox->SetStrings(items);
        }

    //-------------------------------------------
    void InsertGanttChartDlg::OnBarShapeModeChanged()
        {
        const bool allMode = (m_shapeAllRadio != nullptr && m_shapeAllRadio->GetValue());
        if (m_shapeAllChoice != nullptr)
            {
            m_shapeAllChoice->Enable(allMode);
            }
        if (m_shapePerBarListBox != nullptr)
            {
            m_shapePerBarListBox->Enable(!allMode);
            }
        if (allMode)
            {
            OnBarShapeAllChanged();
            }
        else
            {
            SyncBarShapesToList();
            }
        }

    //-------------------------------------------
    void InsertGanttChartDlg::OnBarShapeAllChanged()
        {
        if (m_shapeAllChoice == nullptr)
            {
            return;
            }
        const auto shape = BarShapeFromIndex(m_shapeAllChoice->GetSelection());
        m_barShapes.clear();
        for (const auto& label : m_taskLabels)
            {
            m_barShapes.emplace_back(label, shape);
            }
        SyncBarShapesToList();
        }

    //-------------------------------------------
    Graphs::BarChart::BarShape InsertGanttChartDlg::BarShapeFromIndex(const int index) noexcept
        {
        switch (index)
            {
        case 1:
            return Graphs::BarChart::BarShape::Arrow;
        case 2:
            return Graphs::BarChart::BarShape::ReverseArrow;
        case 0:
            [[fallthrough]];
        default:
            return Graphs::BarChart::BarShape::Rectangle;
            }
        }

    //-------------------------------------------
    int InsertGanttChartDlg::BarShapeToIndex(const Graphs::BarChart::BarShape shape) noexcept
        {
        switch (shape)
            {
        case Graphs::BarChart::BarShape::Arrow:
            return 1;
        case Graphs::BarChart::BarShape::ReverseArrow:
            return 2;
        case Graphs::BarChart::BarShape::Rectangle:
            [[fallthrough]];
        default:
            return 0;
            }
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertGanttChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertGanttChartDlg::GetSelectedDataset() const
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
    bool InsertGanttChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_taskVariable.empty() || m_startDateVariable.empty() || m_endDateVariable.empty())
            {
            wxMessageBox(_(L"Please select the task, start date, and end date variables."),
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
    DateInterval InsertGanttChartDlg::GetDateInterval() const noexcept
        {
        switch (m_dateInterval)
            {
        case 0:
            return DateInterval::Daily;
        case 1:
            return DateInterval::FiscalQuarterly;
        case 2:
            return DateInterval::Monthly;
        case 3:
            return DateInterval::Weekly;
        default:
            return DateInterval::FiscalQuarterly;
            }
        }

    //-------------------------------------------
    FiscalYear InsertGanttChartDlg::GetFiscalYearType() const noexcept
        {
        switch (m_fyType)
            {
        case 0:
            return FiscalYear::Education;
        case 1:
            return FiscalYear::USBusiness;
        default:
            return FiscalYear::USBusiness;
            }
        }

    //-------------------------------------------
    Graphs::GanttChart::TaskLabelDisplay InsertGanttChartDlg::GetTaskLabelDisplay() const noexcept
        {
        switch (m_taskLabelDisplay)
            {
        case 0:
            return Graphs::GanttChart::TaskLabelDisplay::Resource;
        case 1:
            return Graphs::GanttChart::TaskLabelDisplay::Description;
        case 2:
            return Graphs::GanttChart::TaskLabelDisplay::ResourceAndDescription;
        case 3:
            return Graphs::GanttChart::TaskLabelDisplay::Days;
        case 4:
            return Graphs::GanttChart::TaskLabelDisplay::ResourceAndDays;
        case 5:
            return Graphs::GanttChart::TaskLabelDisplay::DescriptionAndDays;
        case 6:
            return Graphs::GanttChart::TaskLabelDisplay::ResourceDescriptionAndDays;
        case 7:
            return Graphs::GanttChart::TaskLabelDisplay::NoDisplay;
        default:
            return Graphs::GanttChart::TaskLabelDisplay::Days;
            }
        }

    //-------------------------------------------
    void InsertGanttChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* gantt = dynamic_cast<const Graphs::GanttChart*>(&graph);
        if (gantt == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = gantt->GetPropertyTemplate(L"dataset");
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

        // load column names from property templates
        // (GanttChart does not store column names as members)
        m_taskVariable = gantt->GetPropertyTemplate(L"variables.task");
        m_startDateVariable = gantt->GetPropertyTemplate(L"variables.start-date");
        m_endDateVariable = gantt->GetPropertyTemplate(L"variables.end-date");
        m_resourceVariable = gantt->GetPropertyTemplate(L"variables.resource");
        m_descriptionVariable = gantt->GetPropertyTemplate(L"variables.description");
        m_completionVariable = gantt->GetPropertyTemplate(L"variables.completion");
        m_groupVariable = gantt->GetPropertyTemplate(L"variables.group");
        UpdateVariableLabels();

        // date interval
        switch (gantt->GetDateDisplayInterval())
            {
        case DateInterval::Daily:
            m_dateInterval = 0;
            break;
        case DateInterval::FiscalQuarterly:
            m_dateInterval = 1;
            break;
        case DateInterval::Monthly:
            m_dateInterval = 2;
            break;
        case DateInterval::Weekly:
            m_dateInterval = 3;
            break;
        default:
            m_dateInterval = 1;
            break;
            }

        // fiscal year type
        switch (gantt->GetFiscalYearType())
            {
        case FiscalYear::Education:
            m_fyType = 0;
            break;
        case FiscalYear::USBusiness:
            m_fyType = 1;
            break;
        default:
            m_fyType = 1;
            break;
            }

        // task label display
        switch (gantt->GetLabelDisplay())
            {
        case Graphs::GanttChart::TaskLabelDisplay::Resource:
            m_taskLabelDisplay = 0;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::Description:
            m_taskLabelDisplay = 1;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::ResourceAndDescription:
            m_taskLabelDisplay = 2;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::Days:
            m_taskLabelDisplay = 3;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::ResourceAndDays:
            m_taskLabelDisplay = 4;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::DescriptionAndDays:
            m_taskLabelDisplay = 5;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::ResourceDescriptionAndDays:
            m_taskLabelDisplay = 6;
            break;
        case Graphs::GanttChart::TaskLabelDisplay::NoDisplay:
            m_taskLabelDisplay = 7;
            break;
        default:
            m_taskLabelDisplay = 3;
            break;
            }

        // showcase bars
        m_ghostOpacity = gantt->GetGhostOpacity();
        m_showcaseBars = gantt->GetShowcasedLabels();
        if (m_showcaseListBox != nullptr)
            {
            wxArrayString strings;
            for (const auto& showBar : m_showcaseBars)
                {
                strings.Add(showBar);
                }
            m_showcaseListBox->SetStrings(strings);
            }

        // capture current bars for the per-bar shape list
        m_taskLabels.clear();
        m_barShapes.clear();
        for (const auto& bar : gantt->GetBars())
            {
            m_taskLabels.push_back(bar.GetAxisLabel().GetText());
            m_barShapes.emplace_back(bar.GetAxisLabel().GetText(), bar.GetShape());
            }

        // determine bar-shape mode from the loaded chart
        bool allSameShape = !gantt->GetBars().empty();
        const auto firstShape =
            gantt->GetBars().empty() ?
                (gantt->GetScalingAxis().IsReversed() ? Graphs::BarChart::BarShape::ReverseArrow :
                                                        Graphs::BarChart::BarShape::Arrow) :
                gantt->GetBars().front().GetShape();
        for (const auto& bar : gantt->GetBars())
            {
            if (bar.GetShape() != firstShape)
                {
                allSameShape = false;
                break;
                }
            }
        if (allSameShape)
            {
            m_barShapeAllIndex = BarShapeToIndex(firstShape);
            if (m_shapeAllRadio != nullptr)
                {
                m_shapeAllRadio->SetValue(true);
                }
            }
        else
            {
            m_barShapeAllIndex = BarShapeToIndex(firstShape);
            if (m_shapePerBarRadio != nullptr)
                {
                m_shapePerBarRadio->SetValue(true);
                }
            }

        TransferDataToWindow();

        // sync bar-shape controls (populates list and sets enable state)
        if (m_shapeAllChoice != nullptr)
            {
            m_shapeAllChoice->Enable(m_shapeAllRadio != nullptr && m_shapeAllRadio->GetValue());
            }
        if (m_shapePerBarListBox != nullptr)
            {
            m_shapePerBarListBox->Enable(m_shapePerBarRadio != nullptr &&
                                         m_shapePerBarRadio->GetValue());
            }
        SyncBarShapesToList();
        }
    } // namespace Wisteria::UI
