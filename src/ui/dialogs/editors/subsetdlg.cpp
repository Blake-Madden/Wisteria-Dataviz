///////////////////////////////////////////////////////////////////////////////
// Name:        subsetdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "subsetdlg.h"
#include <wx/tokenzr.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    SubsetDlg::SubsetDlg(const ReportBuilder* reportBuilder, wxWindow* parent, const wxWindowID id,
                         const wxPoint& pos, const wxSize& size, const long style)
        : wxDialog(parent, id, _(L"Subset Dataset"), pos, size, style),
          m_reportBuilder(reportBuilder)
        {
        CreateControls();

        // start with one empty filter row
        OnAddFilter();

        EnableControlsForFilterType();

        SetMinSize(FromDIP(wxSize{ 550, 600 }));
        SetSize(FromDIP(wxSize{ 600, 700 }));
        Centre();

        SetDefaultOutputName();
        }

    //-------------------------------------------
    SubsetDlg::SubsetDlg(const ReportBuilder* reportBuilder, const SubsetOptions& subsetOptions,
                         wxWindow* parent, const wxWindowID id, const wxPoint& pos,
                         const wxSize& size, const long style)
        : wxDialog(parent, id, _(L"Edit Subset"), pos, size, style), m_reportBuilder(reportBuilder),
          m_mode(Mode::Edit)
        {
        CreateControls();

        // suppress preview updates while populating controls
        m_suppressPreview = true;

        // preselect the source dataset
        const int dsIdx = m_datasetChoice->FindString(subsetOptions.m_sourceDatasetName);
        if (dsIdx != wxNOT_FOUND)
            {
            m_datasetChoice->SetSelection(dsIdx);
            PopulateColumnChoices();
            }

        // set the filter type
        switch (subsetOptions.m_filterType)
            {
        case SubsetOptions::FilterType::Single:
            m_singleRadio->SetValue(true);
            break;
        case SubsetOptions::FilterType::And:
            m_andRadio->SetValue(true);
            break;
        case SubsetOptions::FilterType::Or:
            m_orRadio->SetValue(true);
            break;
        case SubsetOptions::FilterType::Section:
            m_sectionRadio->SetValue(true);
            break;
            }

        // populate filter rows or section fields
        if (subsetOptions.m_filterType != SubsetOptions::FilterType::Section)
            {
            for (const auto& criterion : subsetOptions.m_filters)
                {
                OnAddFilter();
                auto& row = m_filterRows.back();

                const int colIdx = row.m_columnChoice->FindString(criterion.m_column);
                if (colIdx != wxNOT_FOUND)
                    {
                    row.m_columnChoice->SetSelection(colIdx);
                    }

                const int opIdx = row.m_operatorChoice->FindString(criterion.m_operator);
                if (opIdx != wxNOT_FOUND)
                    {
                    row.m_operatorChoice->SetSelection(opIdx);
                    }

                wxString valuesStr;
                for (const auto& val : criterion.m_values)
                    {
                    if (!valuesStr.empty())
                        {
                        valuesStr += L", ";
                        }
                    valuesStr += val;
                    }
                row.m_valuesCtrl->SetValue(valuesStr);
                }
            // ensure at least one row
            if (m_filterRows.empty())
                {
                OnAddFilter();
                }
            }
        else
            {
            // add an empty filter row for if the user switches filter types
            OnAddFilter();

            const int colIdx = m_sectionColumnChoice->FindString(subsetOptions.m_sectionColumn);
            if (colIdx != wxNOT_FOUND)
                {
                m_sectionColumnChoice->SetSelection(colIdx);
                }
            m_startLabelCtrl->SetValue(subsetOptions.m_sectionStartLabel);
            m_endLabelCtrl->SetValue(subsetOptions.m_sectionEndLabel);
            m_includeSentinelsCheck->SetValue(subsetOptions.m_sectionIncludeSentinelLabels);
            }

        m_outputNameCtrl->SetValue(subsetOptions.m_outputName);

        // now allow preview and run it once with all controls populated
        m_suppressPreview = false;
        EnableControlsForFilterType();
        UpdatePreview();

        SetMinSize(FromDIP(wxSize{ 550, 600 }));
        SetSize(FromDIP(wxSize{ 600, 700 }));
        Centre();
        }

    //-------------------------------------------
    void SubsetDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        datasetSizer->AddGrowableCol(1, 1);
        auto* datasetLabel = new wxStaticText(this, wxID_ANY, _(L"Dataset:"));
        datasetSizer->Add(datasetLabel, wxSizerFlags{}.CenterVertical());
        m_datasetChoice = new wxChoice(this, wxID_ANY);
        if (m_reportBuilder != nullptr)
            {
            for (const auto& [name, dataset] : m_reportBuilder->GetDatasets())
                {
                m_datasetNames.push_back(name);
                m_datasetChoice->Append(name);
                }
            }
        if (!m_datasetNames.empty())
            {
            m_datasetChoice->SetSelection(0);
            }
        datasetLabel->Enable(m_mode == Mode::Insert);
        m_datasetChoice->Enable(m_mode == Mode::Insert);
        datasetSizer->Add(m_datasetChoice, wxSizerFlags{}.Expand());
        mainSizer->Add(datasetSizer, wxSizerFlags{}.Expand().Border());

        // filter type radio buttons
        auto* filterTypeBox = new wxStaticBoxSizer(wxHORIZONTAL, this, _(L"Filter Type"));
        m_singleRadio = new wxRadioButton(filterTypeBox->GetStaticBox(), wxID_ANY, _(L"Single"),
                                          wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        m_andRadio = new wxRadioButton(filterTypeBox->GetStaticBox(), wxID_ANY,
                                       _DT(L"AND", DTExplanation::Syntax));
        m_orRadio = new wxRadioButton(filterTypeBox->GetStaticBox(), wxID_ANY,
                                      _DT(L"OR", DTExplanation::Syntax));
        m_sectionRadio = new wxRadioButton(filterTypeBox->GetStaticBox(), wxID_ANY, _(L"Section"));
        m_singleRadio->SetValue(true);
        filterTypeBox->Add(m_singleRadio, wxSizerFlags{}.CenterVertical().Border());
        filterTypeBox->Add(m_andRadio, wxSizerFlags{}.CenterVertical().Border());
        filterTypeBox->Add(m_orRadio, wxSizerFlags{}.CenterVertical().Border());
        filterTypeBox->Add(m_sectionRadio, wxSizerFlags{}.CenterVertical().Border());
        mainSizer->Add(filterTypeBox, wxSizerFlags{}.Expand().Border());

        // filter criteria (Single / AND / OR) — all controls on dialog, no child panel
        m_filterBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Column Filters"));
        m_filterRowsSizer = new wxBoxSizer(wxVERTICAL);
        m_filterBox->Add(m_filterRowsSizer, wxSizerFlags{}.Expand());

        auto* filterBtnSizer = new wxBoxSizer(wxHORIZONTAL);
        m_addFilterBtn = new wxButton(m_filterBox->GetStaticBox(), wxID_ANY, _(L"Add Filter"));
        m_removeFilterBtn =
            new wxButton(m_filterBox->GetStaticBox(), wxID_ANY, _(L"Remove Filter"));
        filterBtnSizer->Add(m_addFilterBtn, wxSizerFlags{}.Border(wxRIGHT));
        filterBtnSizer->Add(m_removeFilterBtn);
        m_filterBox->Add(filterBtnSizer, wxSizerFlags{}.Border(wxTOP));
        mainSizer->Add(m_filterBox, wxSizerFlags{}.Expand().Border());

        // section controls — all controls on dialog, no child panel
        m_sectionBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Section Filter"));
        auto* sectionGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        sectionGrid->AddGrowableCol(1, 1);

        m_sectionColumnLabel =
            new wxStaticText(m_sectionBox->GetStaticBox(), wxID_ANY, _(L"Column:"));
        sectionGrid->Add(m_sectionColumnLabel, wxSizerFlags{}.CenterVertical());
        m_sectionColumnChoice = new wxChoice(m_sectionBox->GetStaticBox(), wxID_ANY);
        sectionGrid->Add(m_sectionColumnChoice, wxSizerFlags{}.Expand());

        m_startLabelLabel =
            new wxStaticText(m_sectionBox->GetStaticBox(), wxID_ANY, _(L"Start label:"));
        sectionGrid->Add(m_startLabelLabel, wxSizerFlags{}.CenterVertical());
        m_startLabelCtrl = new wxComboBox(m_sectionBox->GetStaticBox(), wxID_ANY);
        sectionGrid->Add(m_startLabelCtrl, wxSizerFlags{}.Expand());

        m_endLabelLabel =
            new wxStaticText(m_sectionBox->GetStaticBox(), wxID_ANY, _(L"End label:"));
        sectionGrid->Add(m_endLabelLabel, wxSizerFlags{}.CenterVertical());
        m_endLabelCtrl = new wxComboBox(m_sectionBox->GetStaticBox(), wxID_ANY);
        sectionGrid->Add(m_endLabelCtrl, wxSizerFlags{}.Expand());

        sectionGrid->AddSpacer(0);
        m_includeSentinelsCheck =
            new wxCheckBox(m_sectionBox->GetStaticBox(), wxID_ANY, _(L"Include sentinel labels"));
        m_includeSentinelsCheck->SetValue(true);
        sectionGrid->Add(m_includeSentinelsCheck, wxSizerFlags{}.Expand());

        m_sectionBox->Add(sectionGrid, wxSizerFlags{}.Expand().Border());
        mainSizer->Add(m_sectionBox, wxSizerFlags{}.Expand().Border());

        // output name
        auto* nameSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        nameSizer->AddGrowableCol(1, 1);
        auto* nameLabel = new wxStaticText(this, wxID_ANY, _(L"Output name:"));
        nameSizer->Add(nameLabel, wxSizerFlags{}.CenterVertical());
        m_outputNameCtrl = new wxTextCtrl(this, wxID_ANY, wxString{});
        nameSizer->Add(m_outputNameCtrl, wxSizerFlags{}.Expand());
        mainSizer->Add(nameSizer, wxSizerFlags{}.Expand().Border());
        nameLabel->Enable(m_mode == Mode::Insert);
        m_outputNameCtrl->Enable(m_mode == Mode::Insert);

        // preview grid
        auto* previewBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_previewGrid = new wxGrid(previewBox->GetStaticBox(), wxID_ANY);
        m_previewGrid->CreateGrid(0, 0);
        m_previewGrid->EnableEditing(false);
        m_previewGrid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
        m_previewGrid->SetMinSize(FromDIP(wxSize{ 600, 300 }));
        previewBox->Add(m_previewGrid, wxSizerFlags{ 1 }.Expand().Border());
        auto* previewNote =
            new wxStaticText(previewBox->GetStaticBox(), wxID_ANY,
                             wxString::Format(_(L"Preview is limited to the first %s rows."),
                                              wxNumberFormatter::ToString(
                                                  static_cast<long>(Settings::PREVIEW_MAX_ROWS))));
        previewNote->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
        previewBox->Add(previewNote, wxSizerFlags{}.Border(wxLEFT | wxBOTTOM));
        mainSizer->Add(previewBox, wxSizerFlags{ 1 }.Expand().Border());

        // OK / Cancel
        mainSizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        // populate column choices for the initially selected dataset
        PopulateColumnChoices();

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        m_singleRadio->Bind(wxEVT_RADIOBUTTON,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnFilterTypeChanged(); });
        m_andRadio->Bind(wxEVT_RADIOBUTTON,
                         [this]([[maybe_unused]] wxCommandEvent&) { OnFilterTypeChanged(); });
        m_orRadio->Bind(wxEVT_RADIOBUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnFilterTypeChanged(); });
        m_sectionRadio->Bind(wxEVT_RADIOBUTTON,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnFilterTypeChanged(); });
        m_addFilterBtn->Bind(wxEVT_BUTTON,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnAddFilter(); });
        m_removeFilterBtn->Bind(wxEVT_BUTTON,
                                [this]([[maybe_unused]] wxCommandEvent&) { OnRemoveFilter(); });

        // bind section controls for live preview
        m_sectionColumnChoice->Bind(wxEVT_CHOICE,
                                    [this]([[maybe_unused]] wxCommandEvent&)
                                    {
                                        PopulateSectionLabelChoices();
                                        UpdatePreview();
                                    });
        m_startLabelCtrl->Bind(wxEVT_TEXT,
                               [this]([[maybe_unused]] wxCommandEvent&) { UpdatePreview(); });
        m_endLabelCtrl->Bind(wxEVT_TEXT,
                             [this]([[maybe_unused]] wxCommandEvent&) { UpdatePreview(); });
        m_includeSentinelsCheck->Bind(wxEVT_CHECKBOX, [this]([[maybe_unused]] wxCommandEvent&)
                                      { UpdatePreview(); });
        }

    //-------------------------------------------
    void SubsetDlg::OnDatasetChanged()
        {
        m_subsettedDataset.reset();

        // clear filter rows back to one empty row
        while (m_filterRows.size() > 1)
            {
            OnRemoveFilter();
            }
        if (!m_filterRows.empty())
            {
            m_filterRows.front().m_columnChoice->SetSelection(wxNOT_FOUND);
            m_filterRows.front().m_operatorChoice->SetSelection(0);
            m_filterRows.front().m_valuesCtrl->Clear();
            m_filterRows.front().m_valuesCtrl->SetValue(wxString{});
            }

        m_sectionColumnChoice->SetSelection(wxNOT_FOUND);
        m_startLabelCtrl->Clear();
        m_startLabelCtrl->SetValue(wxString{});
        m_endLabelCtrl->Clear();
        m_endLabelCtrl->SetValue(wxString{});
        m_includeSentinelsCheck->SetValue(true);

        PopulateColumnChoices();
        SetDefaultOutputName();
        ResetPreviewGrid();
        }

    //-------------------------------------------
    void SubsetDlg::OnFilterTypeChanged()
        {
        EnableControlsForFilterType();
        UpdatePreview();
        }

    //-------------------------------------------
    void SubsetDlg::OnAddFilter()
        {
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

        FilterRow row;
        row.m_columnChoice = new wxChoice(m_filterBox->GetStaticBox(), wxID_ANY);
        row.m_operatorChoice = new wxChoice(m_filterBox->GetStaticBox(), wxID_ANY);
        row.m_operatorChoice->Append(
            std::vector<wxString>{ L"=", L"!=", L"<", L"<=", L">", L">=" });
        row.m_operatorChoice->SetSelection(0);
        row.m_valuesCtrl = new wxComboBox(m_filterBox->GetStaticBox(), wxID_ANY, wxString{},
                                          wxDefaultPosition, wxDefaultSize, 0, nullptr);

        // populate column choices from the current dataset
        const auto dataset = GetSelectedDataset();
        if (dataset != nullptr)
            {
            if (dataset->HasValidIdData())
                {
                row.m_columnChoice->Append(dataset->GetIdColumn().GetName());
                }
            for (const auto& name : dataset->GetCategoricalColumnNames())
                {
                row.m_columnChoice->Append(name);
                }
            for (const auto& name : dataset->GetDateColumnNames())
                {
                row.m_columnChoice->Append(name);
                }
            for (const auto& name : dataset->GetContinuousColumnNames())
                {
                row.m_columnChoice->Append(name);
                }
            }

        rowSizer->Add(row.m_columnChoice, wxSizerFlags{ 1 }.CenterVertical().Border(wxRIGHT));
        rowSizer->Add(row.m_operatorChoice, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        rowSizer->Add(row.m_valuesCtrl, wxSizerFlags{ 2 }.CenterVertical());

        m_filterRowsSizer->Add(rowSizer, wxSizerFlags{}.Expand().Border(wxBOTTOM));
        m_filterRows.push_back(row);

        // bind filter row controls for live preview
        row.m_columnChoice->Bind(
            wxEVT_CHOICE,
            [this, rowIndex = m_filterRows.size() - 1]([[maybe_unused]] wxCommandEvent&)
            {
                PopulateFilterValueChoices(m_filterRows[rowIndex]);
                UpdatePreview();
            });
        row.m_operatorChoice->Bind(wxEVT_CHOICE,
                                   [this]([[maybe_unused]] wxCommandEvent&) { UpdatePreview(); });
        row.m_valuesCtrl->Bind(wxEVT_TEXT,
                               [this]([[maybe_unused]] wxCommandEvent&) { UpdatePreview(); });

        EnableControlsForFilterType();
        m_filterBox->GetStaticBox()->Layout();
        Layout();
        }

    //-------------------------------------------
    void SubsetDlg::OnRemoveFilter()
        {
        if (m_filterRows.size() <= 1)
            {
            return;
            }

        m_filterRows.pop_back();

        // remove the last sizer item from the filter rows sizer
        const auto itemCount = m_filterRowsSizer->GetItemCount();
        if (itemCount > 0)
            {
            auto* lastItem = m_filterRowsSizer->GetItem(itemCount - 1);
            if (lastItem != nullptr && lastItem->GetSizer() != nullptr)
                {
                lastItem->GetSizer()->Clear(true);
                }
            m_filterRowsSizer->Remove(static_cast<int>(itemCount - 1));
            }

        EnableControlsForFilterType();
        m_filterBox->GetStaticBox()->Layout();
        Layout();
        }

    //-------------------------------------------
    void SubsetDlg::PopulateColumnChoices()
        {
        const auto dataset = GetSelectedDataset();

        // populate the section column choice (ID and categorical only)
        m_sectionColumnChoice->Clear();
        if (dataset != nullptr)
            {
            if (dataset->HasValidIdData())
                {
                m_sectionColumnChoice->Append(dataset->GetIdColumn().GetName());
                }
            for (const auto& name : dataset->GetCategoricalColumnNames())
                {
                m_sectionColumnChoice->Append(name);
                }
            }

        // repopulate filter row column choices (preserve selections where possible)
        for (auto& row : m_filterRows)
            {
            const wxString currentCol = row.m_columnChoice->GetStringSelection();
            row.m_columnChoice->Clear();
            if (dataset != nullptr)
                {
                if (dataset->HasValidIdData())
                    {
                    row.m_columnChoice->Append(dataset->GetIdColumn().GetName());
                    }
                for (const auto& name : dataset->GetCategoricalColumnNames())
                    {
                    row.m_columnChoice->Append(name);
                    }
                for (const auto& name : dataset->GetDateColumnNames())
                    {
                    row.m_columnChoice->Append(name);
                    }
                for (const auto& name : dataset->GetContinuousColumnNames())
                    {
                    row.m_columnChoice->Append(name);
                    }
                }
            // restore previous selection if still valid
            if (!currentCol.empty())
                {
                const int idx = row.m_columnChoice->FindString(currentCol);
                if (idx != wxNOT_FOUND)
                    {
                    row.m_columnChoice->SetSelection(idx);
                    }
                }
            }
        }

    //-------------------------------------------
    void SubsetDlg::PopulateFilterValueChoices(FilterRow& row)
        {
        row.m_valuesCtrl->Clear();

        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            return;
            }

        const wxString column = row.m_columnChoice->GetStringSelection();
        if (column.empty())
            {
            return;
            }

        const auto catCol = dataset->GetCategoricalColumn(column);
        if (catCol != dataset->GetCategoricalColumns().cend())
            {
            for (const auto& [id, label] : catCol->GetStringTable())
                {
                if (!label.empty())
                    {
                    row.m_valuesCtrl->Append(label);
                    }
                }
            }
        }

    //-------------------------------------------
    void SubsetDlg::PopulateSectionLabelChoices()
        {
        m_startLabelCtrl->Clear();
        m_endLabelCtrl->Clear();

        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            return;
            }

        const wxString column = m_sectionColumnChoice->GetStringSelection();
        if (column.empty())
            {
            return;
            }

        const auto catCol = dataset->GetCategoricalColumn(column);
        if (catCol != dataset->GetCategoricalColumns().cend())
            {
            for (const auto& [id, label] : catCol->GetStringTable())
                {
                if (!label.empty())
                    {
                    m_startLabelCtrl->Append(label);
                    m_endLabelCtrl->Append(label);
                    }
                }
            }
        }

    //-------------------------------------------
    void SubsetDlg::EnableControlsForFilterType()
        {
        const bool isSection = m_sectionRadio->GetValue();
        const bool isSingle = m_singleRadio->GetValue();

        // enable/disable filter criteria controls
        for (size_t i = 0; i < m_filterRows.size(); ++i)
            {
            const bool enabled = !isSection && (!isSingle || i == 0);
            m_filterRows[i].m_columnChoice->Enable(enabled);
            m_filterRows[i].m_operatorChoice->Enable(enabled);
            m_filterRows[i].m_valuesCtrl->Enable(enabled);
            }
        // show add/remove only in AND/OR mode
        const bool isMultiFilter = !isSingle && !isSection;
        m_addFilterBtn->Enable(isMultiFilter);
        m_removeFilterBtn->Enable(isMultiFilter && m_filterRows.size() > 1);

        // enable/disable section controls
        m_sectionColumnLabel->Enable(isSection);
        m_sectionColumnChoice->Enable(isSection);
        m_startLabelLabel->Enable(isSection);
        m_startLabelCtrl->Enable(isSection);
        m_endLabelLabel->Enable(isSection);
        m_endLabelCtrl->Enable(isSection);
        m_includeSentinelsCheck->Enable(isSection);
        }

    //-------------------------------------------
    SubsetOptions::FilterType SubsetDlg::GetSelectedFilterType() const
        {
        if (m_andRadio->GetValue())
            {
            return SubsetOptions::FilterType::And;
            }
        if (m_orRadio->GetValue())
            {
            return SubsetOptions::FilterType::Or;
            }
        if (m_sectionRadio->GetValue())
            {
            return SubsetOptions::FilterType::Section;
            }
        return SubsetOptions::FilterType::Single;
        }

    //-------------------------------------------
    void SubsetDlg::ResetPreviewGrid()
        {
        // replace the grid's table with an empty one to fully clear it
        m_previewGrid->SetTable(new wxGridStringTable(0, 0), true);
        m_previewGrid->ForceRefresh();
        }

    //-------------------------------------------
    void SubsetDlg::UpdatePreview()
        {
        if (m_suppressPreview)
            {
            return;
            }

        m_subsettedDataset.reset();

        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            ResetPreviewGrid();
            return;
            }

        const auto filterType = GetSelectedFilterType();

        try
            {
            Data::Subset dataSubsetter;

            if (filterType == SubsetOptions::FilterType::Section)
                {
                const wxString column = m_sectionColumnChoice->GetStringSelection();
                wxString startLabel = m_startLabelCtrl->GetValue().Strip(wxString::both);
                wxString endLabel = m_endLabelCtrl->GetValue().Strip(wxString::both);

                if (column.empty() || startLabel.empty() || endLabel.empty())
                    {
                    ResetPreviewGrid();
                    return;
                    }

                // expand constant placeholders (e.g., {{MaxFall}})
                if (m_reportBuilder != nullptr)
                    {
                    startLabel = m_reportBuilder->ExpandConstants(startLabel);
                    endLabel = m_reportBuilder->ExpandConstants(endLabel);
                    }

                m_subsettedDataset = dataSubsetter.SubsetSection(
                    dataset, column, startLabel, endLabel, m_includeSentinelsCheck->GetValue());
                }
            else
                {
                // build column filters from the filter rows
                std::vector<Data::ColumnFilterInfo> filters;

                for (const auto& row : m_filterRows)
                    {
                    const wxString column = row.m_columnChoice->GetStringSelection();
                    if (column.empty())
                        {
                        continue;
                        }

                    const wxString opStr = row.m_operatorChoice->GetStringSelection();
                    Comparison cmp = Comparison::Equals;
                    if (opStr == L"!=")
                        {
                        cmp = Comparison::NotEquals;
                        }
                    else if (opStr == L"<")
                        {
                        cmp = Comparison::LessThan;
                        }
                    else if (opStr == L"<=")
                        {
                        cmp = Comparison::LessThanOrEqualTo;
                        }
                    else if (opStr == L">")
                        {
                        cmp = Comparison::GreaterThan;
                        }
                    else if (opStr == L">=")
                        {
                        cmp = Comparison::GreaterThanOrEqualTo;
                        }

                    Data::ColumnFilterInfo cFilter;
                    cFilter.m_columnName = column;
                    cFilter.m_comparisonType = cmp;

                    // parse comma-separated values, using the column type
                    // to determine how to interpret each value
                    const wxString valuesStr = row.m_valuesCtrl->GetValue().Strip(wxString::both);
                    if (valuesStr.empty())
                        {
                        continue;
                        }

                    const bool isContinuous = dataset->GetContinuousColumn(column) !=
                                              dataset->GetContinuousColumns().cend();
                    const bool isDate =
                        dataset->GetDateColumn(column) != dataset->GetDateColumns().cend();

                    wxStringTokenizer tokenizer(valuesStr, L",");
                    while (tokenizer.HasMoreTokens())
                        {
                        wxString token = tokenizer.GetNextToken().Strip(wxString::both);
                        if (token.empty())
                            {
                            continue;
                            }
                        // expand constant placeholders (e.g., {{MaxFall}})
                        if (m_reportBuilder != nullptr)
                            {
                            token = m_reportBuilder->ExpandConstants(token);
                            }
                        if (isContinuous)
                            {
                            double numVal{ 0 };
                            if (token.ToDouble(&numVal))
                                {
                                cFilter.m_values.push_back(numVal);
                                }
                            }
                        else if (isDate)
                            {
                            wxDateTime dt;
                            if (dt.ParseDateTime(token) || dt.ParseDate(token))
                                {
                                cFilter.m_values.push_back(dt);
                                }
                            }
                        else
                            {
                            cFilter.m_values.push_back(Data::DatasetValueType(wxString(token)));
                            }
                        }

                    if (cFilter.m_values.empty())
                        {
                        continue;
                        }
                    filters.push_back(std::move(cFilter));
                    }

                if (filters.empty())
                    {
                    ResetPreviewGrid();
                    return;
                    }

                if (filterType == SubsetOptions::FilterType::Single)
                    {
                    m_subsettedDataset = dataSubsetter.SubsetSimple(dataset, filters.front());
                    }
                else if (filterType == SubsetOptions::FilterType::And)
                    {
                    m_subsettedDataset = dataSubsetter.SubsetAnd(dataset, filters);
                    }
                else
                    {
                    m_subsettedDataset = dataSubsetter.SubsetOr(dataset, filters);
                    }
                }
            }
        catch (const std::exception& exc)
            {
            wxLogDebug(L"Subset preview failed: %s", wxString::FromUTF8(exc.what()));
            ResetPreviewGrid();
            return;
            }
        catch (...)
            {
            ResetPreviewGrid();
            return;
            }

        if (m_subsettedDataset == nullptr)
            {
            ResetPreviewGrid();
            return;
            }

        auto* table = new DatasetGridTable(m_subsettedDataset);
        table->SetMaxRows(Settings::PREVIEW_MAX_ROWS);
        m_previewGrid->SetTable(table, true);
        m_previewGrid->AutoSizeColumns(false);
        m_previewGrid->ForceRefresh();
        }

    //-------------------------------------------
    void SubsetDlg::SetDefaultOutputName()
        {
        const wxString dsName = GetSelectedDatasetName();
        if (dsName.empty())
            {
            m_outputNameCtrl->SetValue(wxString{});
            return;
            }
        const wxString baseName = wxString::Format(_(L"%s (Subset)"), dsName);
        m_outputNameCtrl->SetValue((m_reportBuilder != nullptr) ?
                                       m_reportBuilder->GenerateUniqueDatasetName(baseName) :
                                       baseName);
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> SubsetDlg::GetSelectedDataset() const
        {
        if (m_reportBuilder == nullptr || m_datasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return nullptr;
            }

        const auto& datasets = m_reportBuilder->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    wxString SubsetDlg::GetSelectedDatasetName() const
        {
        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return {};
            }
        return m_datasetNames[sel];
        }

    //-------------------------------------------
    wxString SubsetDlg::GetOutputName() const
        {
        return m_outputNameCtrl->GetValue().Strip(wxString::both);
        }

    //-------------------------------------------
    SubsetOptions SubsetDlg::GetSubsetOptions() const
        {
        SubsetOptions opts;
        opts.m_sourceDatasetName = GetSelectedDatasetName();
        opts.m_outputName = GetOutputName();
        opts.m_filterType = GetSelectedFilterType();

        if (opts.m_filterType != SubsetOptions::FilterType::Section)
            {
            for (const auto& row : m_filterRows)
                {
                SubsetOptions::FilterCriterion criterion;
                criterion.m_column = row.m_columnChoice->GetStringSelection();
                criterion.m_operator = row.m_operatorChoice->GetStringSelection();

                const wxString valuesStr = row.m_valuesCtrl->GetValue().Strip(wxString::both);
                if (!valuesStr.empty())
                    {
                    wxStringTokenizer tokenizer(valuesStr, L",");
                    while (tokenizer.HasMoreTokens())
                        {
                        const wxString token = tokenizer.GetNextToken().Strip(wxString::both);
                        if (!token.empty())
                            {
                            criterion.m_values.push_back(token);
                            }
                        }
                    }

                if (!criterion.m_column.empty() && !criterion.m_values.empty())
                    {
                    opts.m_filters.push_back(std::move(criterion));
                    }
                }
            }
        else
            {
            opts.m_sectionColumn = m_sectionColumnChoice->GetStringSelection();
            opts.m_sectionStartLabel = m_startLabelCtrl->GetValue().Strip(wxString::both);
            opts.m_sectionEndLabel = m_endLabelCtrl->GetValue().Strip(wxString::both);
            opts.m_sectionIncludeSentinelLabels = m_includeSentinelsCheck->GetValue();
            }

        return opts;
        }

    //-------------------------------------------
    bool SubsetDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        const auto filterType = GetSelectedFilterType();

        if (filterType == SubsetOptions::FilterType::Section)
            {
            if (m_sectionColumnChoice->GetSelection() == wxNOT_FOUND)
                {
                wxMessageBox(_(L"Please select a column for the section filter."),
                             _(L"Column Not Specified"), wxOK | wxICON_WARNING, this);
                return false;
                }
            if (m_startLabelCtrl->GetValue().Strip(wxString::both).empty())
                {
                wxMessageBox(_(L"Please specify a start label."), _(L"Label Required"),
                             wxOK | wxICON_WARNING, this);
                return false;
                }
            if (m_endLabelCtrl->GetValue().Strip(wxString::both).empty())
                {
                wxMessageBox(_(L"Please specify an end label."), _(L"Label Required"),
                             wxOK | wxICON_WARNING, this);
                return false;
                }
            }
        else
            {
            // check that at least one filter row has a column and values
            bool hasValidFilter = false;
            for (const auto& row : m_filterRows)
                {
                if (row.m_columnChoice->GetSelection() != wxNOT_FOUND &&
                    !row.m_valuesCtrl->GetValue().Strip(wxString::both).empty())
                    {
                    hasValidFilter = true;
                    break;
                    }
                }
            if (!hasValidFilter)
                {
                wxMessageBox(_(L"Please specify at least one filter with a column and values."),
                             _(L"Filter Required"), wxOK | wxICON_WARNING, this);
                return false;
                }
            }

        if (GetOutputName().empty())
            {
            wxMessageBox(_(L"Please specify an output dataset name."), _(L"Name Required"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        if (m_mode == Mode::Insert && m_reportBuilder != nullptr &&
            m_reportBuilder->GetDatasets().contains(GetOutputName()))
            {
            wxMessageBox(_(L"A dataset with this name already exists. "
                           "Please choose a different output name."),
                         _(L"Duplicate Name"), wxOK | wxICON_WARNING, this);
            return false;
            }

        // attempt to generate the subset to verify the filter works
        UpdatePreview();
        if (m_subsettedDataset == nullptr)
            {
            wxMessageBox(_(L"The subset could not be generated. "
                           "Please verify your filter settings."),
                         _(L"Subset Error"), wxOK | wxICON_WARNING, this);
            return false;
            }

        return true;
        }
    } // namespace Wisteria::UI
