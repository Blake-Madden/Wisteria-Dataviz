///////////////////////////////////////////////////////////////////////////////
// Name:        inserttabledlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "inserttabledlg.h"
#include "../../graphs/table.h"
#include "../../wxSimpleJSON/src/wxSimpleJSON.h"
#include "variableselectdlg.h"
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertTableDlg::InsertTableDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode)
        : InsertGraphDlg(
              canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
              static_cast<GraphDlgOptions>(GraphDlgIncludeMost & ~GraphDlgIncludeColorScheme))
        {
        SetFitRowToContent(true);
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertTableDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Table"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        datasetSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Dataset:")),
                          wxSizerFlags{}.CenterVertical());
        m_datasetChoice = new wxChoice(optionsPage, ID_DATASET_CHOICE);
        datasetSizer->Add(m_datasetChoice);

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

        // two-column layout for everything below the dataset row
        auto* twoColSizer = new wxGridBagSizer(wxSizerFlags::GetDefaultBorder(),
                                               wxSizerFlags::GetDefaultBorder() * 2);

        // left column: columns, header/layout, appearance, sizing
        auto* leftColSizer = new wxBoxSizer(wxVERTICAL);

        // columns static box
        auto* columnBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Columns Selection"));

        // variable selection mode
        wxArrayString varModeChoices;
        varModeChoices.Add(_(L"All columns"));
        varModeChoices.Add(_(L"Custom selection"));
        m_varModeRadio = new wxRadioBox(columnBox->GetStaticBox(), ID_VAR_MODE_RADIO, _(L"Method"),
                                        wxDefaultPosition, wxDefaultSize, varModeChoices, 1,
                                        wxRA_SPECIFY_COLS, wxGenericValidator(&m_varModeIndex));
        columnBox->Add(m_varModeRadio, wxSizerFlags{}.Border().Expand());

        // editable list of column entries (plain names or regex formulas)
        m_variablesListBox = new wxEditableListBox(
            columnBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(250), FromDIP(150) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        columnBox->Add(m_variablesListBox, wxSizerFlags{ 1 }.Expand().Border());

        // variables button adds picks from the variable selector to the list
        m_varButton =
            new wxButton(columnBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Variables..."));
        columnBox->Add(m_varButton, wxSizerFlags{}.Border(wxLEFT | wxBOTTOM));

        leftColSizer->Add(columnBox, wxSizerFlags{ 1 }.Border(wxBOTTOM).Expand());

        // set initial enabled states (All columns selected by default)
        m_variablesListBox->Disable();
        m_varButton->Disable();

        // header & layout
        auto* headerBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Header && Layout"));
        auto* headerParent = headerBox->GetStaticBox();

        headerBox->Add(new wxCheckBox(headerParent, wxID_ANY, _(L"Transpose"), wxDefaultPosition,
                                      wxDefaultSize, 0, wxGenericValidator(&m_transpose)),
                       wxSizerFlags{}.Border());
        headerBox->Add(new wxCheckBox(headerParent, wxID_ANY, _(L"Bold header row"),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      wxGenericValidator(&m_boldHeaderRow)),
                       wxSizerFlags{}.Border());
        headerBox->Add(new wxCheckBox(headerParent, wxID_ANY, _(L"Center header row"),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      wxGenericValidator(&m_centerHeaderRow)),
                       wxSizerFlags{}.Border());
        headerBox->Add(new wxCheckBox(headerParent, wxID_ANY, _(L"Bold first column"),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      wxGenericValidator(&m_boldFirstColumn)),
                       wxSizerFlags{}.Border());

        leftColSizer->Add(headerBox, wxSizerFlags{}.Border(wxBOTTOM).Expand());

        // appearance
        auto* appearBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Appearance"));
        auto* appearParent = appearBox->GetStaticBox();

        auto* altColorSizer = new wxBoxSizer(wxHORIZONTAL);
        altColorSizer->Add(new wxCheckBox(appearParent, wxID_ANY, _(L"Alternate row colors"),
                                          wxDefaultPosition, wxDefaultSize, 0,
                                          wxGenericValidator(&m_alternateRowColors)),
                           wxSizerFlags{}.CenterVertical());
        altColorSizer->AddSpacer(wxSizerFlags::GetDefaultBorder() * 2);
        m_altRowColorPicker = new wxColourPickerCtrl(
            appearParent, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::AzureMist));
        altColorSizer->Add(m_altRowColorPicker, wxSizerFlags{}.CenterVertical());
        appearBox->Add(altColorSizer, wxSizerFlags{}.Border());

        appearBox->Add(new wxCheckBox(appearParent, wxID_ANY, _(L"Clear trailing row formatting"),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      wxGenericValidator(&m_clearTrailingRowFormatting)),
                       wxSizerFlags{}.Border());

        leftColSizer->Add(appearBox, wxSizerFlags{}.Border(wxBOTTOM).Expand());

        // sizing
        auto* sizeBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Sizing"));
        auto* sizeParent = sizeBox->GetStaticBox();

        auto* minWidthSizer = new wxBoxSizer(wxHORIZONTAL);
        minWidthSizer->Add(new wxCheckBox(sizeParent, wxID_ANY, _(L"Minimum width (%):"),
                                          wxDefaultPosition, wxDefaultSize, 0,
                                          wxGenericValidator(&m_useMinWidth)),
                           wxSizerFlags{}.CenterVertical());
        minWidthSizer->AddSpacer(wxSizerFlags::GetDefaultBorder() * 2);
            {
            auto* spin = new wxSpinCtrl(sizeParent, wxID_ANY);
            spin->SetRange(1, 100);
            spin->SetValue(m_minWidthPct);
            spin->SetValidator(wxGenericValidator(&m_minWidthPct));
            minWidthSizer->Add(spin, wxSizerFlags{}.CenterVertical());
            }
        sizeBox->Add(minWidthSizer, wxSizerFlags{}.Border());

        auto* minHeightSizer = new wxBoxSizer(wxHORIZONTAL);
        minHeightSizer->Add(new wxCheckBox(sizeParent, wxID_ANY, _(L"Minimum height (%):"),
                                           wxDefaultPosition, wxDefaultSize, 0,
                                           wxGenericValidator(&m_useMinHeight)),
                            wxSizerFlags{}.CenterVertical());
        minHeightSizer->AddSpacer(wxSizerFlags::GetDefaultBorder() * 2);
            {
            auto* spin = new wxSpinCtrl(sizeParent, wxID_ANY);
            spin->SetRange(1, 100);
            spin->SetValue(m_minHeightPct);
            spin->SetValidator(wxGenericValidator(&m_minHeightPct));
            minHeightSizer->Add(spin, wxSizerFlags{}.CenterVertical());
            }
        sizeBox->Add(minHeightSizer, wxSizerFlags{}.Border());

        leftColSizer->Add(sizeBox, wxSizerFlags{}.Expand());

        twoColSizer->Add(leftColSizer, wxGBPosition(0, 0), wxGBSpan(1, 1),
                         wxEXPAND | wxLEFT | wxBOTTOM, wxSizerFlags::GetDefaultBorder());

        // right column: footnotes
        auto* rightColSizer = new wxBoxSizer(wxVERTICAL);
        auto* footnotesBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Footnotes"));
        m_footnotesListBox = new wxEditableListBox(
            footnotesBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(250), FromDIP(150) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        footnotesBox->Add(m_footnotesListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        m_footnotesListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                                 [this](wxCommandEvent&) { OnAddFootnote(); });
        m_footnotesListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                  [this](wxCommandEvent&) { OnEditFootnote(); });
        m_footnotesListBox->GetDelButton()->Bind(wxEVT_BUTTON,
                                                 [this](wxCommandEvent&) { OnRemoveFootnote(); });
        m_footnotesListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                                 [this](wxListEvent&) { OnEditFootnote(); });

        rightColSizer->Add(footnotesBox, wxSizerFlags{ 1 }.Expand());
        rightColSizer->AddStretchSpacer(1);

        twoColSizer->Add(rightColSizer, wxGBPosition(0, 1), wxGBSpan(1, 1),
                         wxEXPAND | wxRIGHT | wxBOTTOM, wxSizerFlags::GetDefaultBorder());

        twoColSizer->AddGrowableCol(0);
        twoColSizer->AddGrowableCol(1);
        twoColSizer->AddGrowableRow(0);

        optionsSizer->Add(twoColSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxRIGHT));

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        m_varModeRadio->Bind(wxEVT_RADIOBOX,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnVarModeChanged(); });

        m_varButton->Bind(wxEVT_BUTTON,
                          [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertTableDlg::OnVarModeChanged()
        {
        const bool isCustom =
            (static_cast<VarMode>(m_varModeRadio->GetSelection()) == VarMode::Custom);

        m_variablesListBox->Enable(isCustom);
        m_varButton->Enable(isCustom);
        }

    //-------------------------------------------
    void InsertTableDlg::OnDatasetChanged()
        {
        m_variableNames.clear();
        RefreshVariablesList();
        }

    //-------------------------------------------
    void InsertTableDlg::OnSelectVariables()
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
        VariableSelectDlg dlg(this, columnInfo, { VLI{}.Label(_(L"Columns")).Required(true) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // pull any in-place edits from the list before appending
        wxArrayString currentStrings;
        m_variablesListBox->GetStrings(currentStrings);
        m_variableNames.assign(currentStrings.cbegin(), currentStrings.cend());

        for (const auto& name : dlg.GetSelectedVariables(0))
            {
            if (std::ranges::find(m_variableNames, name) == m_variableNames.cend())
                {
                m_variableNames.push_back(name);
                }
            }
        RefreshVariablesList();
        }

    //-------------------------------------------
    void InsertTableDlg::RefreshVariablesList()
        {
        wxArrayString strings;
        strings.reserve(m_variableNames.size());
        for (const auto& name : m_variableNames)
            {
            strings.Add(name);
            }
        m_variablesListBox->SetStrings(strings);
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertTableDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertTableDlg::GetSelectedDataset() const
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
    wxString InsertTableDlg::GetVariableFormula() const
        {
        return (GetVarMode() == VarMode::Everything) ? wxString{ L"{{Everything()}}" } : wxString{};
        }

    //-------------------------------------------
    bool InsertTableDlg::Validate()
        {
        TransferDataFromWindow();

        // sync any in-place edits from the editable list into m_variableNames
        if (m_variablesListBox != nullptr)
            {
            wxArrayString currentStrings;
            m_variablesListBox->GetStrings(currentStrings);
            m_variableNames.clear();
            m_variableNames.reserve(currentStrings.size());
            for (const auto& entry : currentStrings)
                {
                const auto trimmed = wxString{ entry }.Trim().Trim(false);
                if (!trimmed.empty())
                    {
                    m_variableNames.push_back(trimmed);
                    }
                }
            }

        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (GetVarMode() == VarMode::Custom && m_variableNames.empty())
            {
            wxMessageBox(_(L"Please add at least one column or pattern."),
                         _(L"No Variables Selected"), wxOK | wxICON_WARNING, this);
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertTableDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* table = dynamic_cast<const Graphs::Table*>(&graph);
        if (table == nullptr)
            {
            return;
            }

        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = table->GetPropertyTemplate(L"dataset");
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

        // restore variable selection mode from property template. the template
        // may be a raw string formula ("{{Everything()}}"), a JSON array of
        // entries (["col1", "{{Matches(`pat`)}}"]), or a single-element array
        // wrapping a formula (["{{Everything()}}"]).
        auto varsTemplate = table->GetPropertyTemplate(L"variables");
        // strip surrounding quotes if present
        if (varsTemplate.starts_with(L"\"") && varsTemplate.ends_with(L"\"") &&
            varsTemplate.length() >= 2)
            {
            varsTemplate = varsTemplate.substr(1, varsTemplate.length() - 2);
            }

        m_variableNames.clear();
        const auto isEverythingFormula = [](const wxString& val)
        { return val.Lower().starts_with(L"{{everything()"); };

        if (varsTemplate.starts_with(L"[") && varsTemplate.ends_with(L"]"))
            {
            // parse the JSON array into individual entries
            wxString stripped = varsTemplate.substr(1, varsTemplate.length() - 2);
            wxStringTokenizer tkz(stripped, L",");
            std::vector<wxString> entries;
            while (tkz.HasMoreTokens())
                {
                auto token = tkz.GetNextToken().Trim().Trim(false);
                if (token.starts_with(L"\"") && token.ends_with(L"\"") && token.length() >= 2)
                    {
                    token = token.substr(1, token.length() - 2);
                    }
                if (!token.empty())
                    {
                    entries.push_back(token);
                    }
                }

            // a single {{Everything()}} entry means "all columns" mode
            if (entries.size() == 1 && isEverythingFormula(entries.front()))
                {
                m_varModeIndex = static_cast<int>(VarMode::Everything);
                }
            else
                {
                m_varModeIndex = static_cast<int>(VarMode::Custom);
                m_variableNames = std::move(entries);
                }
            }
        else if (isEverythingFormula(varsTemplate))
            {
            m_varModeIndex = static_cast<int>(VarMode::Everything);
            }
        else if (!varsTemplate.empty())
            {
            // a lone legacy formula like {{Matches(`pat`)}} becomes a single
            // custom entry
            m_varModeIndex = static_cast<int>(VarMode::Custom);
            m_variableNames.push_back(varsTemplate);
            }
        else
            {
            m_varModeIndex = static_cast<int>(VarMode::Everything);
            }

        // restore transpose
        const auto transposeTemplate = table->GetPropertyTemplate(L"transpose");
        m_transpose = (transposeTemplate == L"true");

        // restore header/layout options (cached as custom UI properties)
        m_boldHeaderRow = (table->GetPropertyTemplate(L"ui.bold-header-row") == L"true");
        m_centerHeaderRow = (table->GetPropertyTemplate(L"ui.center-header-row") == L"true");
        m_boldFirstColumn = (table->GetPropertyTemplate(L"ui.bold-first-column") == L"true");
        m_clearTrailingRowFormatting = table->IsClearingTrailingRowFormatting();

        // restore alternate row color
        const auto altColorTemplate = table->GetPropertyTemplate(L"alternate-row-color");
        if (!altColorTemplate.empty())
            {
            m_alternateRowColors = true;
            // extract color from {"color":"#RRGGBB"}
            const auto startPos = altColorTemplate.find(L"\"color\"");
            if (startPos != wxString::npos)
                {
                // find the color value after the key
                const auto valStart = altColorTemplate.find(L"\"", startPos + 7);
                if (valStart != wxString::npos)
                    {
                    const auto valEnd = altColorTemplate.find(L"\"", valStart + 1);
                    if (valEnd != wxString::npos)
                        {
                        const wxColour color(
                            altColorTemplate.substr(valStart + 1, valEnd - valStart - 1));
                        if (color.IsOk() && m_altRowColorPicker != nullptr)
                            {
                            m_altRowColorPicker->SetColour(color);
                            }
                        }
                    }
                }
            }

        // restore footnotes
        const auto footnotesTemplate = table->GetPropertyTemplate(L"footnotes");
        if (!footnotesTemplate.empty())
            {
            m_footnotes.clear();
            // make it a root-level node so that it deletes itself when we are done with it
            const auto footnotesNode = wxSimpleJSON::Create(footnotesTemplate, true);
            for (const auto& ftNode : footnotesNode->AsNodes())
                {
                m_footnotes.emplace_back(ftNode->GetProperty(L"value")->AsString(),
                                         ftNode->GetProperty(L"footnote")->AsString());
                }
            }

        // restore min width/height proportions
        const auto& minWidth = table->GetMinWidthProportion();
        if (minWidth.has_value())
            {
            m_useMinWidth = true;
            m_minWidthPct = static_cast<int>(std::round(minWidth.value() * 100.0));
            }
        const auto& minHeight = table->GetMinHeightProportion();
        if (minHeight.has_value())
            {
            m_useMinHeight = true;
            m_minHeightPct = static_cast<int>(std::round(minHeight.value() * 100.0));
            }

        TransferDataToWindow();
        RefreshVariablesList();
        OnVarModeChanged();
        RefreshFootnoteList();
        }

    //-------------------------------------------
    void InsertTableDlg::RefreshFootnoteList()
        {
        wxArrayString strings;
        strings.reserve(m_footnotes.size());
        for (const auto& [value, footnote] : m_footnotes)
            {
            strings.Add(wxString::Format(L"%s \u2014 %s", value, footnote));
            }
        m_footnotesListBox->SetStrings(strings);
        }

    //-------------------------------------------
    void InsertTableDlg::OnAddFootnote()
        {
        wxDialog dlg(this, wxID_ANY, _(L"Add Footnote"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        gridSizer->AddGrowableCol(1, 1);

        gridSizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Value:")),
                       wxSizerFlags{}.CenterVertical());
        auto* valueCtrl = new wxTextCtrl(&dlg, wxID_ANY);
        gridSizer->Add(valueCtrl, wxSizerFlags{}.Expand());

        gridSizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Footnote:")),
                       wxSizerFlags{}.CenterVertical());
        auto* footnoteCtrl = new wxTextCtrl(&dlg, wxID_ANY);
        gridSizer->Add(footnoteCtrl, wxSizerFlags{}.Expand());

        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        topSizer->Add(gridSizer, wxSizerFlags{ 1 }.Expand().Border());
        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);
        dlg.SetMinSize(FromDIP(wxSize{ 320, -1 }));
        dlg.Fit();

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto value = valueCtrl->GetValue().Trim().Trim(false);
        const auto footnote = footnoteCtrl->GetValue().Trim().Trim(false);
        if (value.empty() && footnote.empty())
            {
            return;
            }

        m_footnotes.emplace_back(value, footnote);
        RefreshFootnoteList();
        }

    //-------------------------------------------
    void InsertTableDlg::OnEditFootnote()
        {
        const auto sel = m_footnotesListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                        wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_footnotes.size()))
            {
            return;
            }

        auto& [value, footnote] = m_footnotes[static_cast<size_t>(sel)];

        wxDialog dlg(this, wxID_ANY, _(L"Edit Footnote"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        gridSizer->AddGrowableCol(1, 1);

        gridSizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Value:")),
                       wxSizerFlags{}.CenterVertical());
        auto* valueCtrl = new wxTextCtrl(&dlg, wxID_ANY, value);
        gridSizer->Add(valueCtrl, wxSizerFlags{}.Expand());

        gridSizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Footnote:")),
                       wxSizerFlags{}.CenterVertical());
        auto* footnoteCtrl = new wxTextCtrl(&dlg, wxID_ANY, footnote);
        gridSizer->Add(footnoteCtrl, wxSizerFlags{}.Expand());

        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        topSizer->Add(gridSizer, wxSizerFlags{ 1 }.Expand().Border());
        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);
        dlg.SetMinSize(FromDIP(wxSize{ 320, -1 }));
        dlg.Fit();

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        value = valueCtrl->GetValue().Trim().Trim(false);
        footnote = footnoteCtrl->GetValue().Trim().Trim(false);
        RefreshFootnoteList();
        }

    //-------------------------------------------
    void InsertTableDlg::OnRemoveFootnote()
        {
        const auto sel = m_footnotesListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                        wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_footnotes.size()))
            {
            return;
            }

        m_footnotes.erase(m_footnotes.begin() + sel);
        RefreshFootnoteList();
        }
    } // namespace Wisteria::UI
