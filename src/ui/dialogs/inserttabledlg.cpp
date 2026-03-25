///////////////////////////////////////////////////////////////////////////////
// Name:        inserttabledlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "inserttabledlg.h"
#include "../../graphs/table.h"
#include "variableselectdlg.h"
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
    void InsertTableDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Table Options"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

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

        // columns static box
        auto* columnBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Columns Selection"));

        // variable selection mode
        wxArrayString varModeChoices;
        varModeChoices.Add(_(L"All columns"));
        varModeChoices.Add(_(L"Columns matching pattern"));
        varModeChoices.Add(_(L"All columns except pattern"));
        varModeChoices.Add(_(L"Custom selection"));
        m_varModeRadio = new wxRadioBox(columnBox->GetStaticBox(), ID_VAR_MODE_RADIO, _(L"Method"),
                                        wxDefaultPosition, wxDefaultSize, varModeChoices, 1,
                                        wxRA_SPECIFY_COLS, wxGenericValidator(&m_varModeIndex));
        columnBox->Add(m_varModeRadio, wxSizerFlags{}.Border().Expand());

        // pattern text control
        auto* patternSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        patternSizer->Add(new wxStaticText(columnBox->GetStaticBox(), wxID_ANY, _(L"Pattern:")),
                          wxSizerFlags{}.CenterVertical());
        m_varPatternCtrl =
            new wxTextCtrl(columnBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator(&m_varPattern));
        patternSizer->Add(m_varPatternCtrl, wxSizerFlags{}.CenterVertical().Expand());
        patternSizer->AddGrowableCol(1);
        columnBox->Add(patternSizer, wxSizerFlags{}.Border());

        // variables button and label
        m_varButton =
            new wxButton(columnBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Variables..."));
        columnBox->Add(m_varButton, wxSizerFlags{}.Border(wxLEFT));

        auto* varsLabelCaption =
            new wxStaticText(columnBox->GetStaticBox(), wxID_ANY, _(L"Selected:"));
        varsLabelCaption->SetFont(varsLabelCaption->GetFont().Bold());
        m_varsLabel = new wxStaticText(columnBox->GetStaticBox(), wxID_ANY, wxString{});
        m_varsLabel->SetForegroundColour(GetVariableLabelColor());

        auto* varsLabelSizer = new wxBoxSizer(wxHORIZONTAL);
        varsLabelSizer->Add(varsLabelCaption, wxSizerFlags{}.CenterVertical());
        varsLabelSizer->AddSpacer(FromDIP(8));
        varsLabelSizer->Add(m_varsLabel, wxSizerFlags{}.CenterVertical());
        columnBox->Add(varsLabelSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(columnBox, wxSizerFlags{}.Border());

        // set initial enabled states
        m_varPatternCtrl->Disable();
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

        optionsSizer->Add(headerBox, wxSizerFlags{}.Border());

        // appearance
        auto* appearBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Appearance"));
        auto* appearParent = appearBox->GetStaticBox();

        auto* altColorSizer = new wxBoxSizer(wxHORIZONTAL);
        altColorSizer->Add(new wxCheckBox(appearParent, wxID_ANY, _(L"Alternate row colors"),
                                          wxDefaultPosition, wxDefaultSize, 0,
                                          wxGenericValidator(&m_alternateRowColors)),
                           wxSizerFlags{}.CenterVertical());
        altColorSizer->AddSpacer(FromDIP(8));
        m_altRowColorPicker = new wxColourPickerCtrl(
            appearParent, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::AzureMist));
        altColorSizer->Add(m_altRowColorPicker, wxSizerFlags{}.CenterVertical());
        appearBox->Add(altColorSizer, wxSizerFlags{}.Border());

        appearBox->Add(new wxCheckBox(appearParent, wxID_ANY, _(L"Clear trailing row formatting"),
                                      wxDefaultPosition, wxDefaultSize, 0,
                                      wxGenericValidator(&m_clearTrailingRowFormatting)),
                       wxSizerFlags{}.Border());

        optionsSizer->Add(appearBox, wxSizerFlags{}.Border());

        // sizing
        auto* sizeBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Sizing"));
        auto* sizeParent = sizeBox->GetStaticBox();

        auto* minWidthSizer = new wxBoxSizer(wxHORIZONTAL);
        minWidthSizer->Add(new wxCheckBox(sizeParent, wxID_ANY, _(L"Minimum width (%):"),
                                          wxDefaultPosition, wxDefaultSize, 0,
                                          wxGenericValidator(&m_useMinWidth)),
                           wxSizerFlags{}.CenterVertical());
        minWidthSizer->AddSpacer(FromDIP(8));
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
        minHeightSizer->AddSpacer(FromDIP(8));
            {
            auto* spin = new wxSpinCtrl(sizeParent, wxID_ANY);
            spin->SetRange(1, 100);
            spin->SetValue(m_minHeightPct);
            spin->SetValidator(wxGenericValidator(&m_minHeightPct));
            minHeightSizer->Add(spin, wxSizerFlags{}.CenterVertical());
            }
        sizeBox->Add(minHeightSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(sizeBox, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        m_varModeRadio->Bind(wxEVT_RADIOBOX,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnVarModeChanged(); });

        m_varButton->Bind(wxEVT_BUTTON,
                          [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    void InsertTableDlg::OnVarModeChanged()
        {
        const auto mode = static_cast<VarMode>(m_varModeRadio->GetSelection());
        const bool isPattern = (mode == VarMode::Matches || mode == VarMode::EverythingExcept);
        const bool isCustom = (mode == VarMode::Custom);

        m_varPatternCtrl->Enable(isPattern);
        m_varButton->Enable(isCustom);
        }

    //-------------------------------------------
    void InsertTableDlg::OnDatasetChanged()
        {
        m_variableNames.clear();
        UpdateVariableLabels();
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
        VariableSelectDlg dlg(
            this, columnInfo,
            { VLI{}.Label(_(L"Columns")).Required(true).DefaultVariables(m_variableNames) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_variableNames = dlg.GetSelectedVariables(0);
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertTableDlg::UpdateVariableLabels()
        {
        if (m_variableNames.empty())
            {
            m_varsLabel->SetLabel(wxString{});
            }
        else
            {
            m_varsLabel->SetLabel(
                wxString::Format(wxPLURAL(L"%zu column", L"%zu columns", m_variableNames.size()),
                                 m_variableNames.size()));
            }

        GetSideBarBook()->GetCurrentPage()->Layout();
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
        switch (GetVarMode())
            {
        case VarMode::Everything:
            return L"{{Everything()}}";
        case VarMode::Matches:
            return wxString::Format(L"{{Matches(`%s`)}}", m_varPattern);
        case VarMode::EverythingExcept:
            return wxString::Format(L"{{EverythingExcept(`%s`)}}", m_varPattern);
        case VarMode::Custom:
            [[fallthrough]];
        default:
            return wxString{};
            }
        }

    //-------------------------------------------
    bool InsertTableDlg::Validate()
        {
        TransferDataFromWindow();

        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        const auto mode = GetVarMode();
        if (mode == VarMode::Custom && m_variableNames.empty())
            {
            wxMessageBox(_(L"Please select at least one column."), _(L"No Variables Selected"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        if ((mode == VarMode::Matches || mode == VarMode::EverythingExcept) && m_varPattern.empty())
            {
            wxMessageBox(_(L"Please enter a column name pattern."), _(L"Pattern Required"),
                         wxOK | wxICON_WARNING, this);
            m_varPatternCtrl->SetFocus();
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

        // restore variable selection mode from property template
        // (formulas are stored with surrounding quotes, e.g.,
        // "\"{{Everything()}}\"")
        auto varsTemplate = table->GetPropertyTemplate(L"variables");
        // strip surrounding quotes if present
        if (varsTemplate.starts_with(L"\"") && varsTemplate.ends_with(L"\"") &&
            varsTemplate.length() >= 2)
            {
            varsTemplate = varsTemplate.Mid(1, varsTemplate.length() - 2);
            }
        if (varsTemplate.starts_with(L"{{Everything()"))
            {
            m_varModeIndex = static_cast<int>(VarMode::Everything);
            }
        else if (varsTemplate.starts_with(L"{{Matches("))
            {
            m_varModeIndex = static_cast<int>(VarMode::Matches);
            // extract pattern from {{Matches(`pattern`)}}
            const auto startPos = varsTemplate.find(L'`');
            const auto endPos = varsTemplate.rfind(L'`');
            if (startPos != wxString::npos && endPos != wxString::npos && endPos > startPos)
                {
                m_varPattern = varsTemplate.substr(startPos + 1, endPos - startPos - 1);
                }
            }
        else if (varsTemplate.starts_with(L"{{EverythingExcept("))
            {
            m_varModeIndex = static_cast<int>(VarMode::EverythingExcept);
            const auto startPos = varsTemplate.find(L'`');
            const auto endPos = varsTemplate.rfind(L'`');
            if (startPos != wxString::npos && endPos != wxString::npos && endPos > startPos)
                {
                m_varPattern = varsTemplate.substr(startPos + 1, endPos - startPos - 1);
                }
            }
        else
            {
            m_varModeIndex = static_cast<int>(VarMode::Custom);
            // restore column names from the JSON array template
            // (stored as ["col1", "col2", ...])
            if (!varsTemplate.empty())
                {
                wxString stripped = varsTemplate;
                // remove surrounding brackets
                if (stripped.starts_with(L"["))
                    {
                    stripped = stripped.Mid(1);
                    }
                if (stripped.ends_with(L"]"))
                    {
                    stripped.RemoveLast();
                    }
                wxStringTokenizer tkz(stripped, L",");
                m_variableNames.clear();
                while (tkz.HasMoreTokens())
                    {
                    auto token = tkz.GetNextToken().Trim().Trim(false);
                    // remove surrounding quotes
                    if (token.starts_with(L"\"") && token.ends_with(L"\"") && token.length() >= 2)
                        {
                        token = token.Mid(1, token.length() - 2);
                        }
                    if (!token.empty())
                        {
                        m_variableNames.push_back(token);
                        }
                    }
                }
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
        OnVarModeChanged();
        UpdateVariableLabels();
        }
    } // namespace Wisteria::UI
