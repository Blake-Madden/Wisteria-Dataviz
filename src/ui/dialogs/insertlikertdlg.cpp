///////////////////////////////////////////////////////////////////////////////
// Name:        insertlikertdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertlikertdlg.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertLikertDlg::InsertLikertDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                     wxWindow* parent, const wxString& caption, const wxWindowID id,
                                     const wxPoint& pos, const wxSize& size, const long style,
                                     EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        const auto currentSize = GetSize();
        SetSize(currentSize.GetWidth() * 2, currentSize.GetHeight());
        SetMinSize(wxSize{ currentSize.GetWidth() * 2, currentSize.GetHeight() });

        Centre();
        }

    //-------------------------------------------
    void InsertLikertDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Likert Options"), ID_OPTIONS_SECTION, true);

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
        auto* varButton = new wxButton(optionsPage, ID_SELECT_VARS_BUTTON, _(L"Variables..."));
        optionsSizer->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // variable label grid
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* questionsLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Questions:"));
        questionsLabel->SetFont(questionsLabel->GetFont().Bold());
        varGrid->Add(questionsLabel, wxSizerFlags{}.CenterVertical());
        m_questionsVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_questionsVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_questionsVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Group (optional):"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(varGrid, wxSizerFlags{}.Border());

        // checkboxes
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show response counts"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showResponseCounts)),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show percentages"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showPercentages)),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show section headers"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showSectionHeaders)),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY,
                                         _(L"Adjust bar widths to respondent size"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_adjustBarWidths)),
                          wxSizerFlags{}.Border());

        // colors
        auto* colorSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Negative color:")),
                        wxSizerFlags{}.CenterVertical());
        m_negativeColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::Orange));
        colorSizer->Add(m_negativeColorPicker, wxSizerFlags{}.CenterVertical());

        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Positive color:")),
                        wxSizerFlags{}.CenterVertical());
        m_positiveColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::Cerulean));
        colorSizer->Add(m_positiveColorPicker, wxSizerFlags{}.CenterVertical());

        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Neutral color:")),
                        wxSizerFlags{}.CenterVertical());
        m_neutralColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::AshGrey));
        colorSizer->Add(m_neutralColorPicker, wxSizerFlags{}.CenterVertical());

        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"No-response color:")),
                        wxSizerFlags{}.CenterVertical());
        m_noResponseColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::White));
        colorSizer->Add(m_noResponseColorPicker, wxSizerFlags{}.CenterVertical());

        optionsSizer->Add(colorSizer, wxSizerFlags{}.Border());

        // header labels
        auto* labelSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        labelSizer->AddGrowableCol(1, 1);

        labelSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Positive label:")),
                        wxSizerFlags{}.CenterVertical());
        labelSizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_positiveLabel)),
                        wxSizerFlags{}.CenterVertical().Expand());

        labelSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Negative label:")),
                        wxSizerFlags{}.CenterVertical());
        labelSizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_negativeLabel)),
                        wxSizerFlags{}.CenterVertical().Expand());

        labelSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"No-response label:")),
                        wxSizerFlags{}.CenterVertical());
        labelSizer->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_noResponseLabel)),
                        wxSizerFlags{}.CenterVertical().Expand());

        optionsSizer->Add(labelSizer, wxSizerFlags{}.Border().Expand());

        // question brackets
        m_bracketListBox =
            new wxEditableListBox(optionsPage, wxID_ANY, _(L"Question brackets:"),
                                  wxDefaultPosition, wxSize{ FromDIP(300), FromDIP(100) },
                                  wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT);
        optionsSizer->Add(m_bracketListBox, wxSizerFlags{ 1 }.Expand().Border());

        // override New button to open a structured sub-dialog
        m_bracketListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                if (m_questionVariables.size() < 2)
                    {
                    wxMessageBox(_(L"At least two question variables are needed "
                                   "to define a bracket."),
                                 _(L"Not Enough Variables"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }

                wxDialog dlg(this, wxID_ANY, _(L"Add Question Bracket"), wxDefaultPosition,
                             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
                auto* sizer = new wxBoxSizer(wxVERTICAL);
                auto* grid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
                grid->AddGrowableCol(1, 1);

                wxArrayString questionChoices;
                for (const auto& var : m_questionVariables)
                    {
                    questionChoices.Add(var);
                    }

                grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Title:")),
                          wxSizerFlags{}.CenterVertical());
                auto* titleCtrl = new wxTextCtrl(&dlg, wxID_ANY);
                grid->Add(titleCtrl, wxSizerFlags{}.Expand());

                grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start question:")),
                          wxSizerFlags{}.CenterVertical());
                auto* startCtrl =
                    new wxChoice(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, questionChoices);
                startCtrl->SetSelection(0);
                grid->Add(startCtrl, wxSizerFlags{}.Expand());

                grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End question:")),
                          wxSizerFlags{}.CenterVertical());
                auto* endCtrl =
                    new wxChoice(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, questionChoices);
                endCtrl->SetSelection(static_cast<int>(questionChoices.size()) - 1);
                grid->Add(endCtrl, wxSizerFlags{}.Expand());

                sizer->Add(grid, wxSizerFlags{ 1 }.Expand().Border());
                sizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                           wxSizerFlags{}.Expand().Border());
                dlg.SetSizer(sizer);
                dlg.Fit();
                const auto fitSize = dlg.GetSize();
                dlg.SetMinSize(fitSize);

                if (dlg.ShowModal() != wxID_OK)
                    {
                    return;
                    }

                const auto startSel = startCtrl->GetSelection();
                const auto endSel = endCtrl->GetSelection();
                if (startSel == wxNOT_FOUND || endSel == wxNOT_FOUND)
                    {
                    return;
                    }
                const auto titleVal = titleCtrl->GetValue().Trim(true).Trim(false);

                m_questionBrackets.emplace_back(questionChoices[startSel], questionChoices[endSel],
                                                titleVal);
                SyncBracketsToList();
            });

        // override Edit button to open a structured sub-dialog for the selected item
        m_bracketListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                auto* listCtrl = m_bracketListBox->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_questionBrackets.size()))
                    {
                    return;
                    }

                if (m_questionVariables.size() < 2)
                    {
                    wxMessageBox(_(L"At least two question variables are needed "
                                   "to define a bracket."),
                                 _(L"Not Enough Variables"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }

                auto& bracket = m_questionBrackets[sel];

                wxDialog dlg(this, wxID_ANY, _(L"Edit Question Bracket"), wxDefaultPosition,
                             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
                auto* sizer = new wxBoxSizer(wxVERTICAL);
                auto* grid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
                grid->AddGrowableCol(1, 1);

                wxArrayString questionChoices;
                for (const auto& var : m_questionVariables)
                    {
                    questionChoices.Add(var);
                    }

                grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start question:")),
                          wxSizerFlags{}.CenterVertical());
                auto* startCtrl =
                    new wxChoice(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, questionChoices);
                startCtrl->SetStringSelection(bracket.m_question1);
                grid->Add(startCtrl, wxSizerFlags{}.Expand());

                grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End question:")),
                          wxSizerFlags{}.CenterVertical());
                auto* endCtrl =
                    new wxChoice(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, questionChoices);
                endCtrl->SetStringSelection(bracket.m_question2);
                grid->Add(endCtrl, wxSizerFlags{}.Expand());

                grid->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Title:")),
                          wxSizerFlags{}.CenterVertical());
                auto* titleCtrl = new wxTextCtrl(&dlg, wxID_ANY, bracket.m_title);
                grid->Add(titleCtrl, wxSizerFlags{}.Expand());

                sizer->Add(grid, wxSizerFlags{ 1 }.Expand().Border());
                sizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                           wxSizerFlags{}.Expand().Border());
                dlg.SetSizer(sizer);
                dlg.Fit();
                const auto fitSize = dlg.GetSize();
                dlg.SetMinSize(fitSize);

                if (dlg.ShowModal() != wxID_OK)
                    {
                    return;
                    }

                const auto startSel = startCtrl->GetSelection();
                const auto endSel = endCtrl->GetSelection();
                if (startSel == wxNOT_FOUND || endSel == wxNOT_FOUND)
                    {
                    return;
                    }

                bracket.m_question1 = questionChoices[startSel];
                bracket.m_question2 = questionChoices[endSel];
                bracket.m_title = titleCtrl->GetValue().Trim(true).Trim(false);
                SyncBracketsToList();
            });

        // override Delete button to remove from the backing vector
        m_bracketListBox->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const long sel = m_bracketListBox->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_questionBrackets.size()))
                    {
                    return;
                    }

                m_questionBrackets.erase(m_questionBrackets.begin() + sel);
                SyncBracketsToList();
            });

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    int InsertLikertDlg::SurveyFormatToIndex(
        const Graphs::LikertChart::LikertSurveyQuestionFormat format) noexcept
        {
        using LF = Graphs::LikertChart::LikertSurveyQuestionFormat;
        switch (format)
            {
        case LF::TwoPoint:
            [[fallthrough]];
        case LF::TwoPointCategorized:
            return 1;
        case LF::ThreePoint:
            [[fallthrough]];
        case LF::ThreePointCategorized:
            return 2;
        case LF::FourPoint:
            [[fallthrough]];
        case LF::FourPointCategorized:
            return 3;
        case LF::FivePoint:
            [[fallthrough]];
        case LF::FivePointCategorized:
            return 4;
        case LF::SixPoint:
            [[fallthrough]];
        case LF::SixPointCategorized:
            return 5;
        case LF::SevenPoint:
            [[fallthrough]];
        case LF::SevenPointCategorized:
            return 6;
        default:
            return 0;
            }
        }

    //-------------------------------------------
    void InsertLikertDlg::OnDatasetChanged()
        {
        m_questionVariables.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertLikertDlg::OnSelectVariables()
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
                  .Label(_(L"Questions"))
                  .SingleSelection(false)
                  .Required(true)
                  .DefaultVariables(m_questionVariables)
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete }),
              VLI{}
                  .Label(_(L"Group"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric,
                                   Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_questionVariables = dlg.GetSelectedVariables(0);

        const auto groupVars = dlg.GetSelectedVariables(1);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertLikertDlg::UpdateVariableLabels()
        {
        if (m_questionVariables.empty())
            {
            m_questionsVarLabel->SetLabel(wxString{});
            }
        else if (m_questionVariables.size() == 1)
            {
            m_questionsVarLabel->SetLabel(m_questionVariables.front());
            }
        else
            {
            m_questionsVarLabel->SetLabel(
                wxString::Format(_(L"%zu columns"), m_questionVariables.size()));
            }
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertLikertDlg::SyncBracketsToList()
        {
        wxArrayString items;
        for (const auto& bracket : m_questionBrackets)
            {
            items.Add(bracket.m_question1 + L" \u2192 " + bracket.m_question2 + L": " +
                      bracket.m_title);
            }
        m_bracketListBox->SetStrings(items);
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertLikertDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertLikertDlg::GetSelectedDataset() const
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
    wxColour InsertLikertDlg::GetNegativeColor() const
        {
        return (m_negativeColorPicker != nullptr) ?
                   m_negativeColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::Orange);
        }

    //-------------------------------------------
    wxColour InsertLikertDlg::GetPositiveColor() const
        {
        return (m_positiveColorPicker != nullptr) ?
                   m_positiveColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::Cerulean);
        }

    //-------------------------------------------
    wxColour InsertLikertDlg::GetNeutralColor() const
        {
        return (m_neutralColorPicker != nullptr) ?
                   m_neutralColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::AshGrey);
        }

    //-------------------------------------------
    wxColour InsertLikertDlg::GetNoResponseColor() const
        {
        return (m_noResponseColorPicker != nullptr) ?
                   m_noResponseColorPicker->GetColour() :
                   Colors::ColorBrewer::GetColor(Colors::Color::White);
        }

    //-------------------------------------------
    bool InsertLikertDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_questionVariables.empty())
            {
            wxMessageBox(_(L"Please select at least one question variable."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertLikertDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* likert = dynamic_cast<const Graphs::LikertChart*>(&graph);
        if (likert == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = likert->GetPropertyTemplate(L"dataset");
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

        // load question variable names from property templates
        m_questionVariables.clear();
        for (size_t i = 0;; ++i)
            {
            const auto varName =
                likert->GetPropertyTemplate(L"variables.questions[" + std::to_wstring(i) + L"]");
            if (varName.empty())
                {
                break;
                }
            m_questionVariables.push_back(varName);
            }
        m_groupVariable = likert->GetPropertyTemplate(L"variables.group");
        UpdateVariableLabels();

        // boolean options
        m_showResponseCounts = likert->IsShowingResponseCounts();
        m_showPercentages = likert->IsShowingPercentages();
        m_showSectionHeaders = likert->IsShowingSectionHeaders();
        m_adjustBarWidths = likert->IsSettingBarSizesToRespondentSize();

        // colors
        if (m_negativeColorPicker != nullptr)
            {
            m_negativeColorPicker->SetColour(likert->GetNegativeColor());
            }
        if (m_positiveColorPicker != nullptr)
            {
            m_positiveColorPicker->SetColour(likert->GetPositiveColor());
            }
        if (m_neutralColorPicker != nullptr)
            {
            m_neutralColorPicker->SetColour(likert->GetNeutralColor());
            }
        if (m_noResponseColorPicker != nullptr)
            {
            m_noResponseColorPicker->SetColour(likert->GetNoResponseColor());
            }

        // header labels
        m_positiveLabel = likert->GetPositiveHeader();
        m_negativeLabel = likert->GetNegativeHeader();
        m_noResponseLabel = likert->GetNoResponseHeader();

        // question brackets
        m_questionBrackets.clear();
        for (const auto& bracket : likert->GetQuestionsBrackets())
            {
            m_questionBrackets.push_back(bracket);
            }
        SyncBracketsToList();

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
