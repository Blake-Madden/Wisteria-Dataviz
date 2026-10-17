///////////////////////////////////////////////////////////////////////////////
// Name:        insert_ishikawa_diagram_dlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insert_ishikawa_diagram_dlg.h"
#include "../../app/wisteriaview.h"
#include <set>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    // sub-causes sub-dialog, which lists a branch's causes and lets the user
    // pick the column of sub-causes for each one
    //---------------------------------------------
    class EditSubCausesDlg : public wxDialog
        {
      public:
        EditSubCausesDlg(wxWindow* parent, const std::shared_ptr<Data::Dataset>& dataset,
                         const Graphs::IshikawaDiagram::Branch& branch)
            : wxDialog(parent, wxID_ANY, _(L"Sub-causes"), wxDefaultPosition, wxDefaultSize,
                       wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
            {
            // the first choice means that the sub-causes are typed in
            wxArrayString choices{ _(L"(typed sub-causes)") };
            if (dataset != nullptr)
                {
                for (const auto& column : dataset->GetCategoricalColumns())
                    {
                    choices.Add(column.GetName());
                    }
                }

            // the causes are the branch's current causes
            std::set<wxString> seen;
            if (branch.m_column.empty())
                {
                for (const auto& cause : branch.m_constants)
                    {
                    if (!cause.empty() && seen.insert(cause).second)
                        {
                        m_causes.push_back(cause);
                        }
                    }
                }
            else if (dataset != nullptr)
                {
                const auto causesColumn = dataset->GetCategoricalColumn(branch.m_column);
                if (causesColumn != dataset->GetCategoricalColumns().cend())
                    {
                    for (size_t row = 0; row < dataset->GetRowCount(); ++row)
                        {
                        const wxString cause{ causesColumn->GetValueAsLabel(row) };
                        if (!cause.empty() && seen.insert(cause).second)
                            {
                            m_causes.push_back(cause);
                            }
                        }
                    }
                }

            auto* mainSizer = new wxBoxSizer(wxVERTICAL);
            auto* scroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize{ FromDIP(720), FromDIP(240) });
            scroller->SetScrollRate(0, FromDIP(10));
            auto* grid = new wxFlexGridSizer(4, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                        wxSizerFlags::GetDefaultBorder() });
            grid->AddGrowableCol(2, 1);
            grid->AddGrowableCol(3, 1);

            for (const wxString& header :
                 { _(L"Cause"), _(L"Sub-causes column"), _(L"Typed sub-causes"), _(L"Sub-causes") })
                {
                auto* headerText = new wxStaticText(scroller, wxID_ANY, header);
                headerText->SetFont(headerText->GetFont().Bold());
                grid->Add(headerText);
                }

            // the sub-causes that the row will use, shown so that a wrong pick is easy to spot
            const auto updatePreview =
                [dataset](const wxChoice* choice, const wxTextCtrl* text, wxStaticText* preview)
            {
                std::vector<wxString> subCauses;
                if (choice->GetSelection() > 0)
                    {
                    if (dataset != nullptr)
                        {
                        const auto column =
                            dataset->GetCategoricalColumn(choice->GetStringSelection());
                        if (column != dataset->GetCategoricalColumns().cend())
                            {
                            for (size_t row = 0; row < dataset->GetRowCount(); ++row)
                                {
                                if (const wxString value{ column->GetValueAsLabel(row) };
                                    !value.empty())
                                    {
                                    subCauses.push_back(value);
                                    }
                                }
                            }
                        }
                    }
                else
                    {
                    subCauses = Graphs::IshikawaDiagram::ParseConstants(text->GetValue());
                    }
                wxString shown;
                for (const auto& subCause : subCauses)
                    {
                    if (!shown.empty())
                        {
                        shown += L", ";
                        }
                    shown += subCause;
                    }
                preview->SetLabel(shown.empty() ? _(L"(none)") : shown);
            };

            for (const auto& cause : m_causes)
                {
                grid->Add(new wxStaticText(scroller, wxID_ANY, cause),
                          wxSizerFlags{}.CenterVertical());
                auto* choice =
                    new wxChoice(scroller, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
                auto* text = new wxTextCtrl(scroller, wxID_ANY);
                auto* preview = new wxStaticText(scroller, wxID_ANY, wxString{});
                choice->SetSelection(0);
                const auto existing =
                    std::ranges::find_if(branch.m_subCauses, [&cause](const auto& subCauses)
                                         { return subCauses.m_cause == cause; });
                if (existing != branch.m_subCauses.cend())
                    {
                    const int found{ choices.Index(existing->m_column) };
                    choice->SetSelection(
                        (existing->m_column.empty() || found == wxNOT_FOUND) ? 0 : found);
                    text->SetValue(Graphs::IshikawaDiagram::FormatConstants(existing->m_constants));
                    }
                else
                    {
                    // a column named after the cause is most likely its sub-causes
                    for (int index = 1; std::cmp_less(index, choices.size()); ++index)
                        {
                        if (choices[index].CmpNoCase(cause) == 0)
                            {
                            choice->SetSelection(index);
                            break;
                            }
                        }
                    }
                text->Enable(choice->GetSelection() == 0);
                updatePreview(choice, text, preview);
                choice->Bind(
                    wxEVT_CHOICE,
                    [choice, text, preview, updatePreview]([[maybe_unused]] wxCommandEvent&)
                    {
                        text->Enable(choice->GetSelection() == 0);
                        updatePreview(choice, text, preview);
                    });
                text->Bind(wxEVT_TEXT,
                           [choice, text, preview, updatePreview]([[maybe_unused]] wxCommandEvent&)
                           { updatePreview(choice, text, preview); });
                grid->Add(choice, wxSizerFlags{}.Expand());
                grid->Add(text, wxSizerFlags{}.Expand());
                grid->Add(preview, wxSizerFlags{}.CenterVertical());
                m_choices.push_back(choice);
                m_texts.push_back(text);
                }
            if (m_causes.empty())
                {
                grid->Add(new wxStaticText(scroller, wxID_ANY,
                                           _(L"This branch has no causes. Select its "
                                             "causes column or type its causes first.")));
                grid->AddSpacer(0);
                grid->AddSpacer(0);
                grid->AddSpacer(0);
                }
            scroller->SetSizer(grid);

            mainSizer->Add(scroller, wxSizerFlags{ 1 }.Expand().Border());
            mainSizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                           wxSizerFlags{}.Expand().Border());
            SetSizerAndFit(mainSizer);
            Centre();
            }

        [[nodiscard]]
        std::vector<Graphs::IshikawaDiagram::SubCauses> GetSubCauses() const
            {
            std::vector<Graphs::IshikawaDiagram::SubCauses> result;
            for (size_t i = 0; i < m_causes.size(); ++i)
                {
                if (m_choices[i]->GetSelection() > 0)
                    {
                    result.push_back({ m_causes[i], m_choices[i]->GetStringSelection(), {} });
                    }
                else if (auto typed =
                             Graphs::IshikawaDiagram::ParseConstants(m_texts[i]->GetValue());
                         !typed.empty())
                    {
                    result.push_back({ m_causes[i], wxString{}, std::move(typed) });
                    }
                }
            return result;
            }

      private:
        std::vector<wxString> m_causes;
        std::vector<wxChoice*> m_choices;
        std::vector<wxTextCtrl*> m_texts;
        };

    //-------------------------------------------
    InsertIshikawaDiagramDlg::InsertIshikawaDiagramDlg(Canvas* canvas,
                                                       const ReportBuilder* reportBuilder,
                                                       wxWindow* parent, const wxString& caption,
                                                       const wxWindowID id, const wxPoint& pos,
                                                       const wxSize& size, const long style,
                                                       EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeColorScheme)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    DataGuide InsertIshikawaDiagramDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Ishikawa Diagram");
        guide.m_iconName = L"images/ishikawa.svg";
        guide.m_summary = _(L"Shows the possible causes of a problem, grouped into categories.");
        guide.m_requirements =
            _(L"One categorical column of causes for each branch. Optionally, a column of "
              "sub-causes for any cause.");
        guide.m_shape = DataGuideShape::Wide;
        guide.m_parameters = {
            { .m_name = _(L"Causes"),
              .m_types = { Data::Dataset::ColumnImportType::String,
                           Data::Dataset::ColumnImportType::Discrete },
              .m_requirement = DataGuideRequirement::Required,
              .m_description = _(L"One column for each branch. Each cell is a cause of the "
                                 "effect."),
              .m_values = {},
              .m_multiple = true,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Sub-causes"),
              .m_types = { Data::Dataset::ColumnImportType::String,
                           Data::Dataset::ColumnImportType::Discrete },
              .m_requirement = DataGuideRequirement::Optional,
              .m_description = _(L"A column of sub-causes for one of the causes."),
              .m_values = {},
              .m_multiple = true,
              .m_condition = wxString{},
              .m_group = wxString{} }
        };

        DataGuideExample causes;
        causes.m_title = _(L"Example: causes of surface flaws");
        causes.m_columns = { { _(L"Man"), _(L"Causes") },
                             { _(L"Machine"), _(L"Causes") },
                             { _(L"Material"), _(L"Causes") },
                             { _(L"Training Details"), _(L"Sub-causes") } };
        causes.m_rows = { { _(L"Training"), _(L"Speed"), _(L"Alloy"), _(L"Basic") },
                          { _(L"Fatigue"), _(L"Age"), _(L"Supplier"), _(L"Advanced") },
                          { wxString{}, wxString{}, _(L"Moisture"), wxString{} } };
        guide.m_examples = { std::move(causes) };

        guide.m_missingData = { _(L"Blank cells are skipped.") };
        return guide;
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Ishikawa Diagram"), ID_OPTIONS_SECTION, true);

        auto* grid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        grid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Dataset:")),
                  wxSizerFlags{}.CenterVertical());
        auto* datasetChoice = CreateDatasetChoice(optionsPage);
        grid->Add(datasetChoice);

        grid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Effect:")),
                  wxSizerFlags{}.CenterVertical());
        // the effect can span several lines
        auto* effectCtrl =
            new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                           wxSize{ FromDIP(250), FromDIP(56) }, wxTE_MULTILINE | wxTE_RICH2,
                           wxTextValidator{ wxFILTER_NONE, &m_effect });
#if wxUSE_SPELLCHECK
        effectCtrl->EnableProofCheck(wxTextProofOptions::Default());
#endif
        grid->Add(effectCtrl);
        optionsSizer->Add(grid, wxSizerFlags{}.Border());

        // branches, which start as the six standard categories
        auto* branchesBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Branches"));
        m_branchPanel = new wxScrolledWindow(branchesBox->GetStaticBox(), wxID_ANY,
                                             wxDefaultPosition, wxSize{ -1, FromDIP(220) });
        m_branchPanel->SetScrollRate(0, FromDIP(10));
        m_branchGrid = new wxFlexGridSizer(
            5, wxSize{ wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder() });
        m_branchGrid->AddGrowableCol(2, 1);
        m_branchPanel->SetSizer(m_branchGrid);
        branchesBox->Add(m_branchPanel, wxSizerFlags{ 1 }.Expand().Border());

        auto* addBranchBtn = new wxButton(branchesBox->GetStaticBox(), wxID_ANY, _(L"Add Branch"));
        branchesBox->Add(addBranchBtn, wxSizerFlags{}.Border(wxLEFT | wxBOTTOM));
        optionsSizer->Add(branchesBox, wxSizerFlags{ 1 }.Expand().Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show empty branches"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showEmptyBranches }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show branch labels"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showBranchLabels }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show cause counts"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_showCauseCounts }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Tapered lines"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_taperedLines }),
                          wxSizerFlags{}.Border());
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Monochrome"), wxDefaultPosition,
                                         wxDefaultSize, 0, wxGenericValidator{ &m_monochrome }),
                          wxSizerFlags{}.Border());

        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        addBranchBtn->Bind(wxEVT_BUTTON,
                           [this]([[maybe_unused]] wxCommandEvent&) { OnAddBranch(); });

        // LoadFromGraph() will overwrite these for edit mode
        for (size_t i = 0; i < Graphs::IshikawaDiagram::DEFAULT_BRANCH_COUNT; ++i)
            {
            m_branches.push_back(
                { Graphs::IshikawaDiagram::GetDefaultBranchLabel(i), wxString{}, {} });
            }
        RebuildBranchRows();

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    wxArrayString InsertIshikawaDiagramDlg::GetColumnNames() const
        {
        wxArrayString names;
        if (const auto dataset = GetSelectedDataset(); dataset != nullptr)
            {
            for (const auto& column : dataset->GetCategoricalColumns())
                {
                names.Add(column.GetName());
                }
            }
        return names;
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::RebuildBranchRows()
        {
        const wxWindowUpdateLocker noUpdates(m_branchPanel);
        m_branchGrid->Clear(true);

        // the first choice means that the causes are typed in
        wxArrayString choices{ _(L"(typed causes)") };
        const auto columnNames{ GetColumnNames() };
        for (const auto& name : columnNames)
            {
            choices.Add(name);
            }

        for (const wxString& header :
             { _(L"Label"), _(L"Causes column"), _(L"Typed causes"), wxString{}, wxString{} })
            {
            auto* headerText = new wxStaticText(m_branchPanel, wxID_ANY, header);
            headerText->SetFont(headerText->GetFont().Bold());
            m_branchGrid->Add(headerText);
            }

        for (size_t i = 0; i < m_branches.size(); ++i)
            {
            auto* labelCtrl = new wxTextCtrl(m_branchPanel, wxID_ANY, m_branches[i].m_label);
            labelCtrl->Bind(wxEVT_TEXT,
                            [this, i, labelCtrl]([[maybe_unused]] wxCommandEvent&)
                            {
                                if (i < m_branches.size())
                                    {
                                    m_branches[i].m_label = labelCtrl->GetValue();
                                    }
                            });
            m_branchGrid->Add(labelCtrl, wxSizerFlags{}.Expand());

            auto* columnChoice =
                new wxChoice(m_branchPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
            const int found{ choices.Index(m_branches[i].m_column) };
            columnChoice->SetSelection(
                (m_branches[i].m_column.empty() || found == wxNOT_FOUND) ? 0 : found);
            columnChoice->Bind(wxEVT_CHOICE,
                               [this, i, columnChoice]([[maybe_unused]] wxCommandEvent&)
                               {
                                   const int selection{ columnChoice->GetSelection() };
                                   CallAfter([this, i, selection]()
                                             { OnBranchColumnChanged(i, selection); });
                               });
            m_branchGrid->Add(columnChoice, wxSizerFlags{}.Expand());

            auto* subButton = new wxButton(m_branchPanel, wxID_ANY, _(L"Sub-causes..."));
            subButton->Enable(!m_branches[i].m_column.empty() ||
                              !m_branches[i].m_constants.empty());
            subButton->Bind(wxEVT_BUTTON,
                            [this, i]([[maybe_unused]] wxCommandEvent&) { OnEditSubCauses(i); });

            auto* causesText =
                new wxTextCtrl(m_branchPanel, wxID_ANY,
                               Graphs::IshikawaDiagram::FormatConstants(m_branches[i].m_constants));
            causesText->Enable(m_branches[i].m_column.empty());
            causesText->Bind(wxEVT_TEXT,
                             [this, i, causesText, subButton]([[maybe_unused]] wxCommandEvent&)
                             {
                                 if (i >= m_branches.size())
                                     {
                                     return;
                                     }
                                 auto& branch{ m_branches[i] };
                                 branch.m_constants = Graphs::IshikawaDiagram::ParseConstants(
                                     causesText->GetValue());
                                 // sub-causes only apply to causes that still exist
                                 std::erase_if(branch.m_subCauses,
                                               [&branch](const auto& subCauses)
                                               {
                                                   return std::ranges::find(branch.m_constants,
                                                                            subCauses.m_cause) ==
                                                          branch.m_constants.cend();
                                               });
                                 subButton->Enable(!branch.m_constants.empty());
                             });
            m_branchGrid->Add(causesText, wxSizerFlags{}.Expand());
            m_branchGrid->Add(subButton);

            auto* removeButton = new wxButton(m_branchPanel, wxID_ANY, _(L"Remove"));
            removeButton->Bind(wxEVT_BUTTON, [this, i]([[maybe_unused]] wxCommandEvent&)
                               { CallAfter([this, i]() { OnRemoveBranch(i); }); });
            m_branchGrid->Add(removeButton);
            }

        m_branchPanel->FitInside();
        m_branchPanel->Layout();
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnAddBranch()
        {
        m_branches.push_back(
            { Graphs::IshikawaDiagram::GetDefaultBranchLabel(m_branches.size()), wxString{}, {} });
        RebuildBranchRows();
        m_branchPanel->Scroll(0, m_branchPanel->GetScrollRange(wxVERTICAL));
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnRemoveBranch(const size_t index)
        {
        if (index >= m_branches.size())
            {
            return;
            }
        m_branches.erase(m_branches.begin() + static_cast<std::ptrdiff_t>(index));
        RebuildBranchRows();
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnBranchColumnChanged(const size_t index, const int selection)
        {
        if (index >= m_branches.size())
            {
            return;
            }
        const auto names{ GetColumnNames() };
        const wxString newColumn{ (selection > 0 && std::cmp_less(selection - 1, names.size())) ?
                                      names[static_cast<size_t>(selection - 1)] :
                                      wxString{} };
        // picking what is already selected changes nothing
        if (newColumn == m_branches[index].m_column)
            {
            return;
            }
        // the causes changed, so any sub-causes no longer apply
        m_branches[index].m_subCauses.clear();
        m_branches[index].m_constants.clear();
        m_branches[index].m_column = newColumn;
        RebuildBranchRows();
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnEditSubCauses(const size_t index)
        {
        if (index >= m_branches.size())
            {
            return;
            }
        EditSubCausesDlg dlg(this, GetSelectedDataset(), m_branches[index]);
        if (dlg.ShowModal() == wxID_OK)
            {
            m_branches[index].m_subCauses = dlg.GetSubCauses();
            }
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnDatasetChanged()
        {
        // the columns belong to the previous dataset
        const auto names{ GetColumnNames() };
        for (size_t branchIndex = 0; branchIndex < m_branches.size(); ++branchIndex)
            {
            auto& branch{ m_branches[branchIndex] };
            // a branch with typed causes stays typed, since those don't depend on the dataset
            if (branch.m_column.empty() && !branch.m_constants.empty())
                {
                continue;
                }
            // the sub-causes belonged to the previous dataset's causes
            branch.m_subCauses.clear();
            branch.m_column.clear();
            // reselect the column that is named after the branch
            for (const auto& name : names)
                {
                if ((!branch.m_label.empty() && name.CmpNoCase(branch.m_label) == 0) ||
                    Graphs::IshikawaDiagram::IsDefaultBranchName(branchIndex, name))
                    {
                    branch.m_column = name;
                    break;
                    }
                }
            }
        // typed branches may still have sub-causes that name columns of the previous dataset
        RemoveStaleSubCauses();
        RebuildBranchRows();
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::RemoveStaleSubCauses()
        {
        const auto dataset{ GetSelectedDataset() };
        const auto columnNames{ GetColumnNames() };
        for (auto& branch : m_branches)
            {
            // the causes that sub-causes can belong to
            std::set<wxString> causes;
            if (branch.m_column.empty())
                {
                causes.insert(branch.m_constants.cbegin(), branch.m_constants.cend());
                }
            else if (dataset != nullptr)
                {
                const auto causesColumn = dataset->GetCategoricalColumn(branch.m_column);
                if (causesColumn != dataset->GetCategoricalColumns().cend())
                    {
                    for (size_t row = 0; row < dataset->GetRowCount(); ++row)
                        {
                        causes.insert(causesColumn->GetValueAsLabel(row));
                        }
                    }
                }

            std::erase_if(branch.m_subCauses,
                          [&causes, &columnNames](const auto& subCauses)
                          {
                              return !causes.contains(subCauses.m_cause) ||
                                     (!subCauses.m_column.empty() &&
                                      columnNames.Index(subCauses.m_column) == wxNOT_FOUND);
                          });
            }
        }

    //-------------------------------------------
    bool InsertIshikawaDiagramDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_branches.empty() ||
            std::ranges::none_of(
                m_branches, [](const auto& branch)
                { return !branch.m_column.empty() || !branch.m_constants.empty(); }))
            {
            wxMessageBox(_(L"Please select a causes column, or type the causes, for at least "
                           "one branch."),
                         _(L"Branches Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        // a sub-cause that can't be applied would make building the graph fail
        // after the dialog has closed, so drop those here
        RemoveStaleSubCauses();

        // build the diagram once to make sure that it can be created, so that any problem
        // (e.g., a column that isn't in the dataset) is reported here
        try
            {
            Graphs::IshikawaDiagram trial{ GetCanvas() };
            trial.SetData(GetSelectedDataset(), m_effect, m_branches);
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Invalid Diagram"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* diagram = dynamic_cast<const Graphs::IshikawaDiagram*>(&graph);
        if (diagram == nullptr)
            {
            return;
            }

        LoadGraphOptions(graph);
        SelectDataset(diagram->GetPropertyTemplate(L"dataset"));

        m_effect = diagram->GetEffect();
        m_showEmptyBranches = diagram->IsShowingEmptyBranches();
        m_showBranchLabels = diagram->IsShowingBranchLabels();
        m_showCauseCounts = diagram->IsShowingCauseCounts();
        m_taperedLines = diagram->IsTaperedLines();
        m_monochrome = diagram->IsMonochrome();
        m_branches = diagram->GetBranches();
        RebuildBranchRows();

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::IshikawaDiagram>
    InsertIshikawaDiagramDlg::BuildIshikawaDiagram(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::IshikawaDiagram>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->ShowEmptyBranches(m_showEmptyBranches);
        plot->ShowBranchLabels(m_showBranchLabels);
        plot->ShowCauseCounts(m_showCauseCounts);
        plot->SetTaperedLines(m_taperedLines);
        plot->SetMonochrome(m_monochrome);
        plot->SetData(GetSelectedDataset(), GetEffect(), GetBranches());

        if (oldGraph != nullptr)
            {
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            }

        return plot;
        }
    } // namespace Wisteria::UI
