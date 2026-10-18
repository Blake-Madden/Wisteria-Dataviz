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
            wxArrayString columnNames;
            if (dataset != nullptr)
                {
                for (const auto& column : dataset->GetCategoricalColumns())
                    {
                    columnNames.Add(column.GetName());
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

            const wxArrayString modes{ _(L"In column"), _(L"Constants") };

            auto* mainSizer = new wxBoxSizer(wxVERTICAL);
            auto* scroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize{ FromDIP(720), FromDIP(240) });
            scroller->SetScrollRate(0, FromDIP(10));
            auto* grid = new wxFlexGridSizer(4, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                        wxSizerFlags::GetDefaultBorder() });
            grid->AddGrowableCol(1, 1);
            grid->AddGrowableCol(2, 1);

            for (const wxString& header :
                 { _(L"Cause"), _(L"Mode"), _(L"Sub-causes"), _(L"Preview") })
                {
                auto* headerText = new wxStaticText(scroller, wxID_ANY, header);
                headerText->SetFont(headerText->GetFont().Bold());
                grid->Add(headerText);
                }

            const auto updatePreview = [dataset](const wxComboBox* combo, wxStaticText* preview)
            {
                std::vector<wxString> subCauses;
                if (combo->IsEditable())
                    {
                    subCauses = Graphs::IshikawaDiagram::ParseConstants(combo->GetValue());
                    }
                else if (dataset != nullptr)
                    {
                    const auto column = dataset->GetCategoricalColumn(combo->GetValue());
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

                auto* modeChoice = new wxChoice(scroller, wxID_ANY, wxDefaultPosition,
                                                FromDIP(wxSize{ 120, -1 }), modes);
                modeChoice->SetMinSize(FromDIP(wxSize{ 120, 24 }));

                auto* valueCombo =
                    new wxComboBox(scroller, wxID_ANY, wxString{}, wxDefaultPosition,
                                   FromDIP(wxSize{ 200, -1 }), wxArrayString{}, wxCB_DROPDOWN);
                valueCombo->SetMinSize(FromDIP(wxSize{ 200, 24 }));

                auto* preview = new wxStaticText(scroller, wxID_ANY, wxString{});

                const auto existing =
                    std::ranges::find_if(branch.m_subCauses, [&cause](const auto& subCauses)
                                         { return subCauses.m_cause == cause; });
                if (existing != branch.m_subCauses.cend())
                    {
                    if (existing->m_column.empty())
                        {
                        modeChoice->SetSelection(1);
                        valueCombo->SetValue(
                            Graphs::IshikawaDiagram::FormatConstants(existing->m_constants));
                        valueCombo->SetEditable(true);
                        }
                    else
                        {
                        modeChoice->SetSelection(0);
                        valueCombo->Append(columnNames);
                        valueCombo->SetValue(existing->m_column);
                        valueCombo->SetEditable(false);
                        }
                    }
                else
                    {
                    // default to a column only when one is named after the cause;
                    // otherwise default to Constants (blank)
                    int matchingColumn{ wxNOT_FOUND };
                    for (int index = 0; std::cmp_less(index, columnNames.size()); ++index)
                        {
                        if (columnNames[index].CmpNoCase(cause) == 0)
                            {
                            matchingColumn = index;
                            break;
                            }
                        }
                    if (matchingColumn != wxNOT_FOUND)
                        {
                        modeChoice->SetSelection(0);
                        valueCombo->Append(columnNames);
                        valueCombo->SetValue(columnNames[matchingColumn]);
                        valueCombo->SetEditable(false);
                        }
                    else
                        {
                        modeChoice->SetSelection(1);
                        valueCombo->SetEditable(true);
                        }
                    }

                valueCombo->Enable(true);
                updatePreview(valueCombo, preview);

                modeChoice->Bind(wxEVT_CHOICE,
                                 [modeChoice, valueCombo, preview, updatePreview,
                                  columnNames]([[maybe_unused]] wxCommandEvent&)
                                 {
                                     if (modeChoice->GetSelection() == 0)
                                         {
                                         valueCombo->Clear();
                                         valueCombo->Append(columnNames);
                                         if (!columnNames.empty())
                                             {
                                             valueCombo->SetValue(columnNames[0]);
                                             }
                                         valueCombo->SetEditable(false);
                                         }
                                     else
                                         {
                                         valueCombo->Clear();
                                         valueCombo->SetEditable(true);
                                         }
                                     updatePreview(valueCombo, preview);
                                 });
                valueCombo->Bind(wxEVT_COMBOBOX, [valueCombo, preview,
                                                  updatePreview]([[maybe_unused]] wxCommandEvent&)
                                 { updatePreview(valueCombo, preview); });
                valueCombo->Bind(wxEVT_TEXT, [valueCombo, preview,
                                              updatePreview]([[maybe_unused]] wxCommandEvent&)
                                 { updatePreview(valueCombo, preview); });

                grid->Add(modeChoice, wxSizerFlags{}.Expand());
                grid->Add(valueCombo, wxSizerFlags{}.Expand());
                grid->Add(preview, wxSizerFlags{}.CenterVertical());

                m_modeChoices.push_back(modeChoice);
                m_valueCombos.push_back(valueCombo);
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
                if (!m_valueCombos[i]->IsEditable())
                    {
                    result.push_back({ m_causes[i], m_valueCombos[i]->GetValue(), {} });
                    }
                else if (auto typed =
                             Graphs::IshikawaDiagram::ParseConstants(m_valueCombos[i]->GetValue());
                         !typed.empty())
                    {
                    result.push_back({ m_causes[i], wxString{}, std::move(typed) });
                    }
                }
            return result;
            }

      private:
        std::vector<wxString> m_causes;
        std::vector<wxChoice*> m_modeChoices;
        std::vector<wxComboBox*> m_valueCombos;
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
        InsertGraphDlg::FinalizeControls();

        wxTopLevelWindowBase::SetMinSize(GetSize());

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
        m_branchSizer = new wxFlexGridSizer(
            5, wxSize{ wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder() });
        m_branchSizer->AddGrowableCol(1, 0);
        m_branchSizer->AddGrowableCol(2, 1);
        m_branchPanel->SetSizer(m_branchSizer);
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

        auto* fishStyleSizer = new wxBoxSizer(wxHORIZONTAL);
        fishStyleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Fish style:")),
                            wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        const wxArrayString fishStyleChoices{ _(L"None"), _(L"Fish"), _(L"Shark") };
        auto* fishStyleChoice =
            new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, FromDIP(wxSize{ 120, -1 }),
                         fishStyleChoices, 0, wxGenericValidator{ &m_fishStyle });
        fishStyleChoice->SetSelection(m_fishStyle);
        fishStyleChoice->Bind(wxEVT_CHOICE,
                              [this, fishStyleChoice]([[maybe_unused]] wxCommandEvent&)
                              { m_fishStyle = fishStyleChoice->GetSelection(); });
        fishStyleSizer->Add(fishStyleChoice);
        optionsSizer->Add(fishStyleSizer, wxSizerFlags{}.Border());

        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        addBranchBtn->Bind(wxEVT_BUTTON,
                           [this]([[maybe_unused]] wxCommandEvent&) { OnAddBranch(); });

        // LoadFromGraph() will overwrite these for edit mode
        for (size_t i = 0; i < Graphs::IshikawaDiagram::DEFAULT_BRANCH_COUNT; ++i)
            {
            m_branches.push_back(
                { Graphs::IshikawaDiagram::GetDefaultBranchLabel(i), wxString{}, {}, {} });
            }
        RebuildBranchRows();

        // map the branches to the pre-selected dataset's columns (e.g., a column
        // named "Man" becomes the "People" branch); the choice's initial selection
        // doesn't fire an event, so this has to be done once up-front
        OnDatasetChanged();

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
        m_branchSizer->Clear(true);

        const auto columnNames = GetColumnNames();

        for (const wxString& header :
             { _(L"Label"), _(L"Mode"), _(L"Causes"), wxString{}, wxString{} })
            {
            auto* headerText = new wxStaticText(m_branchPanel, wxID_ANY, header);
            headerText->SetFont(headerText->GetFont().Bold());
            m_branchSizer->Add(headerText);
            }

        const wxArrayString modes{ _(L"In column"), _(L"Constants") };

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
            labelCtrl->SetMinSize(FromDIP(wxSize{ 180, 24 }));
            m_branchSizer->Add(labelCtrl, wxSizerFlags{}.Expand());

            const bool inColumn{ !m_branches[i].m_column.empty() };
            auto* modeChoice = new wxChoice(m_branchPanel, wxID_ANY, wxDefaultPosition,
                                            FromDIP(wxSize{ 90, -1 }), modes);
            modeChoice->SetMinSize(FromDIP(wxSize{ 170, 24 }));
            modeChoice->SetSelection(inColumn ? 0 : 1);
            m_branchSizer->Add(modeChoice, wxSizerFlags{}.Expand());

            auto* valueCombo =
                new wxComboBox(m_branchPanel, wxID_ANY, wxString{}, wxDefaultPosition,
                               FromDIP(wxSize{ 400, -1 }), wxArrayString{}, wxCB_DROPDOWN);
            valueCombo->SetMinSize(FromDIP(wxSize{ 400, 24 }));
            if (inColumn)
                {
                valueCombo->Append(columnNames);
                valueCombo->SetValue(m_branches[i].m_column);
                valueCombo->SetEditable(false);
                }
            else
                {
                valueCombo->SetValue(
                    Graphs::IshikawaDiagram::FormatConstants(m_branches[i].m_constants));
                valueCombo->SetEditable(true);
                }
            m_branchSizer->Add(valueCombo, wxSizerFlags{}.Expand());

            auto* subButton = new wxButton(
                m_branchPanel, wxID_ANY,
                m_branches[i].m_subCauses.empty() ?
                    _(L"Sub-causes...") :
                    wxString::Format(_(L"Sub-causes (%zu)..."), m_branches[i].m_subCauses.size()));
            subButton->Enable(!m_branches[i].m_column.empty() ||
                              !m_branches[i].m_constants.empty());
            subButton->Bind(wxEVT_BUTTON,
                            [this, i]([[maybe_unused]] wxCommandEvent&) { OnEditSubCauses(i); });
            subButton->SetMinSize(FromDIP(wxSize{ 210, 24 }));
            m_branchSizer->Add(subButton);

            auto* removeButton = new wxButton(m_branchPanel, wxID_ANY, _(L"Remove"));
            removeButton->SetMinSize(FromDIP(wxSize{ 130, 24 }));
            removeButton->Bind(wxEVT_BUTTON, [this, i]([[maybe_unused]] wxCommandEvent&)
                               { CallAfter([this, i]() { OnRemoveBranch(i); }); });
            m_branchSizer->Add(removeButton);

            modeChoice->Bind(
                wxEVT_CHOICE,
                [this, i, modeChoice, valueCombo, subButton,
                 columnNames]([[maybe_unused]] wxCommandEvent&)
                {
                    if (i >= m_branches.size())
                        {
                        return;
                        }
                    auto& branch{ m_branches[i] };
                    if (modeChoice->GetSelection() == 0)
                        {
                        branch.m_constants.clear();
                        valueCombo->Clear();
                        valueCombo->Append(columnNames);
                        if (columnNames.Index(branch.m_column) != wxNOT_FOUND)
                            {
                            valueCombo->SetValue(branch.m_column);
                            }
                        else if (!columnNames.empty())
                            {
                            valueCombo->SetValue(columnNames[0]);
                            branch.m_column = columnNames[0];
                            }
                        else
                            {
                            valueCombo->SetValue(wxString{});
                            branch.m_column.clear();
                            }
                        valueCombo->SetEditable(false);
                        }
                    else
                        {
                        branch.m_column.clear();
                        valueCombo->Clear();
                        valueCombo->SetValue(
                            Graphs::IshikawaDiagram::FormatConstants(branch.m_constants));
                        valueCombo->SetEditable(true);
                        }
                    subButton->Enable(!branch.m_column.empty() || !branch.m_constants.empty());
                });
            valueCombo->Bind(wxEVT_COMBOBOX,
                             [this, i, valueCombo, subButton]([[maybe_unused]] wxCommandEvent&)
                             {
                                 if (i >= m_branches.size())
                                     {
                                     return;
                                     }
                                 auto& branch{ m_branches[i] };
                                 branch.m_column = valueCombo->GetValue();
                                 branch.m_constants.clear();
                                 subButton->Enable(!branch.m_column.empty());
                             });
            valueCombo->Bind(
                wxEVT_TEXT,
                [this, i, valueCombo, subButton]([[maybe_unused]] wxCommandEvent&)
                {
                    if (i >= m_branches.size())
                        {
                        return;
                        }
                    auto& branch{ m_branches[i] };
                    if (valueCombo->IsEditable())
                        {
                        branch.m_constants =
                            Graphs::IshikawaDiagram::ParseConstants(valueCombo->GetValue());
                        branch.m_column.clear();
                        std::erase_if(branch.m_subCauses,
                                      [&branch](const auto& subCauses)
                                      {
                                          return std::ranges::find(branch.m_constants,
                                                                   subCauses.m_cause) ==
                                                 branch.m_constants.cend();
                                      });
                        subButton->Enable(!branch.m_constants.empty());
                        subButton->SetLabel(branch.m_subCauses.empty() ?
                                                _(L"Sub-causes...") :
                                                wxString::Format(_(L"Sub-causes (%zu)..."),
                                                                 branch.m_subCauses.size()));
                        }
                });
            }

        m_branchPanel->FitInside();
        m_branchPanel->Layout();
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnAddBranch()
        {
        m_branches.push_back({ Graphs::IshikawaDiagram::GetDefaultBranchLabel(m_branches.size()),
                               wxString{},
                               {},
                               {} });
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
            // refresh the button labels with their new counts
            RebuildBranchRows();
            }
        }

    //-------------------------------------------
    void InsertIshikawaDiagramDlg::OnDatasetChanged()
        {
        // the columns belong to the previous dataset
        const auto names = GetColumnNames();
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
        const auto columnNames = GetColumnNames();
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
        m_fishStyle = static_cast<int>(diagram->GetFishStyle());
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
        plot->SetFishStyle(static_cast<Graphs::IshikawaDiagram::FishStyle>(m_fishStyle));
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
