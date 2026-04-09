///////////////////////////////////////////////////////////////////////////////
// Name:        joindlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "joindlg.h"

namespace Wisteria::UI
    {
    //-------------------------------------------
    JoinDlg::JoinDlg(const ReportBuilder* reportBuilder, wxWindow* parent, const wxWindowID id,
                     const wxPoint& pos, const wxSize& size, const long style)
        : wxDialog(parent, id, _(L"Join Datasets"), pos, size, style),
          m_reportBuilder(reportBuilder)
        {
        CreateControls();

        // start with one empty by-column pair
        OnAddByColumn();

        SetMinSize(FromDIP(wxSize{ 550, 650 }));
        SetSize(FromDIP(wxSize{ 600, 800 }));
        Centre();

        SetDefaultOutputName();
        }

    //-------------------------------------------
    JoinDlg::JoinDlg(const ReportBuilder* reportBuilder, const JoinOptions& joinOptions,
                     wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size,
                     const long style)
        : wxDialog(parent, id, _(L"Edit Join"), pos, size, style), m_reportBuilder(reportBuilder),
          m_mode(Mode::Edit)
        {
        CreateControls();

        // preselect the left dataset
        const int leftIdx = m_leftDatasetChoice->FindString(joinOptions.m_sourceDatasetName);
        if (leftIdx != wxNOT_FOUND)
            {
            m_leftDatasetChoice->SetSelection(leftIdx);
            }

        // preselect the right dataset
        const int rightIdx = m_rightDatasetChoice->FindString(joinOptions.m_otherDatasetName);
        if (rightIdx != wxNOT_FOUND)
            {
            m_rightDatasetChoice->SetSelection(rightIdx);
            }

        // set the join type
        switch (joinOptions.m_type)
            {
        case JoinOptions::JoinType::LeftJoinUniqueLast:
            m_joinTypeChoice->SetSelection(0);
            break;
        case JoinOptions::JoinType::LeftJoinUniqueFirst:
            m_joinTypeChoice->SetSelection(1);
            break;
        case JoinOptions::JoinType::LeftJoin:
            m_joinTypeChoice->SetSelection(2);
            break;
        case JoinOptions::JoinType::InnerJoin:
            m_joinTypeChoice->SetSelection(3);
            break;
            }

        m_suffixCtrl->SetValue(joinOptions.m_suffix);
        m_outputNameCtrl->SetValue(joinOptions.m_outputName);

        // populate column choices before adding by-column rows
        PopulateColumnChoices();

        // add by-column pair rows
        for (const auto& [leftCol, rightCol] : joinOptions.m_byColumns)
            {
            OnAddByColumn();
            auto& row = m_byColumnRows.back();

            const int leftColIdx = row.m_leftColumnChoice->FindString(leftCol);
            if (leftColIdx != wxNOT_FOUND)
                {
                row.m_leftColumnChoice->SetSelection(leftColIdx);
                }

            const int rightColIdx = row.m_rightColumnChoice->FindString(rightCol);
            if (rightColIdx != wxNOT_FOUND)
                {
                row.m_rightColumnChoice->SetSelection(rightColIdx);
                }
            }

        // ensure at least one row
        if (m_byColumnRows.empty())
            {
            OnAddByColumn();
            }

        UpdatePreview();

        SetMinSize(FromDIP(wxSize{ 550, 650 }));
        SetSize(FromDIP(wxSize{ 600, 800 }));
        Centre();
        }

    //-------------------------------------------
    void JoinDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        // dataset selectors
        auto* datasetSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        datasetSizer->AddGrowableCol(1, 1);

        auto* leftLabel = new wxStaticText(this, wxID_ANY, _(L"Left dataset:"));
        datasetSizer->Add(leftLabel, wxSizerFlags{}.CenterVertical());
        m_leftDatasetChoice = new wxChoice(this, wxID_ANY);

        auto* rightLabel = new wxStaticText(this, wxID_ANY, _(L"Right dataset:"));

        if (m_reportBuilder != nullptr)
            {
            for (const auto& [name, dataset] : m_reportBuilder->GetDatasets())
                {
                m_datasetNames.push_back(name);
                m_leftDatasetChoice->Append(name);
                }
            }
        if (!m_datasetNames.empty())
            {
            m_leftDatasetChoice->SetSelection(0);
            }
        leftLabel->Enable(m_mode == Mode::Insert);
        m_leftDatasetChoice->Enable(m_mode == Mode::Insert);
        datasetSizer->Add(m_leftDatasetChoice, wxSizerFlags{}.Expand());

        datasetSizer->Add(rightLabel, wxSizerFlags{}.CenterVertical());
        m_rightDatasetChoice = new wxChoice(this, wxID_ANY);
        for (const auto& name : m_datasetNames)
            {
            m_rightDatasetChoice->Append(name);
            }
        if (m_datasetNames.size() > 1)
            {
            m_rightDatasetChoice->SetSelection(1);
            }
        else if (!m_datasetNames.empty())
            {
            m_rightDatasetChoice->SetSelection(0);
            }
        rightLabel->Enable(m_mode == Mode::Insert);
        m_rightDatasetChoice->Enable(m_mode == Mode::Insert);
        datasetSizer->Add(m_rightDatasetChoice, wxSizerFlags{}.Expand());

        mainSizer->Add(datasetSizer, wxSizerFlags{}.Expand().Border());

        // join type
        auto* joinTypeSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        joinTypeSizer->AddGrowableCol(1, 1);
        joinTypeSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Join type:")),
                           wxSizerFlags{}.CenterVertical());
        m_joinTypeChoice = new wxChoice(this, wxID_ANY);
        m_joinTypeChoice->Append(std::vector<wxString>{ _(L"Left Join (Unique Last)"),
                                                        _(L"Left Join (Unique First)"),
                                                        _(L"Left Join"), _(L"Inner Join") });
        m_joinTypeChoice->SetSelection(0);
        joinTypeSizer->Add(m_joinTypeChoice, wxSizerFlags{}.Expand());
        mainSizer->Add(joinTypeSizer, wxSizerFlags{}.Expand().Border());

        // by-column pairs
        m_byColumnsBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Join Columns"));
        m_byColumnRowsSizer = new wxBoxSizer(wxVERTICAL);

        // column headers
        auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* leftHeader =
            new wxStaticText(m_byColumnsBox->GetStaticBox(), wxID_ANY, _(L"Left Column"));
        leftHeader->SetFont(leftHeader->GetFont().Bold());
        auto* rightHeader =
            new wxStaticText(m_byColumnsBox->GetStaticBox(), wxID_ANY, _(L"Right Column"));
        rightHeader->SetFont(rightHeader->GetFont().Bold());
        headerSizer->Add(leftHeader, wxSizerFlags{ 1 }.Border(wxRIGHT));
        headerSizer->Add(rightHeader, wxSizerFlags{ 1 });
        m_byColumnsBox->Add(headerSizer, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT));

        m_byColumnsBox->Add(m_byColumnRowsSizer, wxSizerFlags{}.Expand());

        auto* byColumnBtnSizer = new wxBoxSizer(wxHORIZONTAL);
        m_addByColumnBtn =
            new wxButton(m_byColumnsBox->GetStaticBox(), wxID_ANY, _(L"Add Column Pair"));
        m_removeByColumnBtn =
            new wxButton(m_byColumnsBox->GetStaticBox(), wxID_ANY, _(L"Remove Column Pair"));
        m_removeByColumnBtn->Enable(false);
        byColumnBtnSizer->Add(m_addByColumnBtn, wxSizerFlags{}.Border(wxRIGHT));
        byColumnBtnSizer->Add(m_removeByColumnBtn);
        m_byColumnsBox->Add(byColumnBtnSizer, wxSizerFlags{}.Border(wxTOP));
        mainSizer->Add(m_byColumnsBox, wxSizerFlags{}.Expand().Border());

        // options
        auto* optionsBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Options"));
        auto* optGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        optGrid->AddGrowableCol(1, 1);

        optGrid->Add(
            new wxStaticText(optionsBox->GetStaticBox(), wxID_ANY, _(L"Duplicate column suffix:")),
            wxSizerFlags{}.CenterVertical());
        m_suffixCtrl = new wxTextCtrl(optionsBox->GetStaticBox(), wxID_ANY, L".x");
        optGrid->Add(m_suffixCtrl, wxSizerFlags{}.Expand());

        optionsBox->Add(optGrid, wxSizerFlags{}.Expand().Border());
        mainSizer->Add(optionsBox, wxSizerFlags{}.Expand().Border());

        // output name
        auto* nameSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
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

        // populate column choices for the initially selected datasets
        PopulateColumnChoices();

        // bind events
        m_leftDatasetChoice->Bind(wxEVT_CHOICE, [this]([[maybe_unused]] wxCommandEvent&)
                                  { OnLeftDatasetChanged(); });
        m_rightDatasetChoice->Bind(wxEVT_CHOICE, [this]([[maybe_unused]] wxCommandEvent&)
                                   { OnRightDatasetChanged(); });
        m_joinTypeChoice->Bind(wxEVT_CHOICE,
                               [this]([[maybe_unused]] wxCommandEvent&) { UpdatePreview(); });
        m_addByColumnBtn->Bind(wxEVT_BUTTON,
                               [this]([[maybe_unused]] wxCommandEvent&) { OnAddByColumn(); });
        m_removeByColumnBtn->Bind(wxEVT_BUTTON,
                                  [this]([[maybe_unused]] wxCommandEvent&) { OnRemoveByColumn(); });
        }

    //-------------------------------------------
    void JoinDlg::OnLeftDatasetChanged()
        {
        m_joinedDataset.reset();
        PopulateColumnChoices();
        SetDefaultOutputName();
        ResetPreviewGrid();
        }

    //-------------------------------------------
    void JoinDlg::OnRightDatasetChanged()
        {
        m_joinedDataset.reset();
        PopulateColumnChoices();
        SetDefaultOutputName();
        ResetPreviewGrid();
        }

    //-------------------------------------------
    void JoinDlg::OnAddByColumn()
        {
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

        ByColumnRow row;
        row.m_leftColumnChoice = new wxChoice(m_byColumnsBox->GetStaticBox(), wxID_ANY);
        row.m_rightColumnChoice = new wxChoice(m_byColumnsBox->GetStaticBox(), wxID_ANY);

        // populate from current datasets
        const auto leftDataset = GetSelectedLeftDataset();
        if (leftDataset != nullptr)
            {
            if (leftDataset->HasValidIdData())
                {
                row.m_leftColumnChoice->Append(leftDataset->GetIdColumn().GetName());
                }
            for (const auto& name : leftDataset->GetCategoricalColumnNames())
                {
                row.m_leftColumnChoice->Append(name);
                }
            }

        const auto rightDataset = GetSelectedRightDataset();
        if (rightDataset != nullptr)
            {
            if (rightDataset->HasValidIdData())
                {
                row.m_rightColumnChoice->Append(rightDataset->GetIdColumn().GetName());
                }
            for (const auto& name : rightDataset->GetCategoricalColumnNames())
                {
                row.m_rightColumnChoice->Append(name);
                }
            }

        rowSizer->Add(row.m_leftColumnChoice, wxSizerFlags{ 1 }.CenterVertical().Border(wxRIGHT));
        rowSizer->Add(row.m_rightColumnChoice, wxSizerFlags{ 1 }.CenterVertical());

        m_byColumnRowsSizer->Add(rowSizer, wxSizerFlags{}.Expand().Border(wxBOTTOM));
        m_byColumnRows.push_back(row);

        // bind for live preview
        row.m_leftColumnChoice->Bind(wxEVT_CHOICE,
                                     [this]([[maybe_unused]] wxCommandEvent&) { UpdatePreview(); });
        row.m_rightColumnChoice->Bind(wxEVT_CHOICE, [this]([[maybe_unused]] wxCommandEvent&)
                                      { UpdatePreview(); });

        m_removeByColumnBtn->Enable(m_byColumnRows.size() > 1);
        m_byColumnsBox->GetStaticBox()->Layout();
        Layout();
        }

    //-------------------------------------------
    void JoinDlg::OnRemoveByColumn()
        {
        if (m_byColumnRows.size() <= 1)
            {
            return;
            }

        m_byColumnRows.pop_back();

        // remove the last sizer item from the by-column rows sizer
        const auto itemCount = m_byColumnRowsSizer->GetItemCount();
        if (itemCount > 0)
            {
            auto* lastItem = m_byColumnRowsSizer->GetItem(itemCount - 1);
            if (lastItem != nullptr && lastItem->GetSizer() != nullptr)
                {
                lastItem->GetSizer()->Clear(true);
                }
            m_byColumnRowsSizer->Remove(static_cast<int>(itemCount - 1));
            }

        m_removeByColumnBtn->Enable(m_byColumnRows.size() > 1);
        m_byColumnsBox->GetStaticBox()->Layout();
        Layout();
        }

    //-------------------------------------------
    void JoinDlg::PopulateColumnChoices()
        {
        const auto leftDataset = GetSelectedLeftDataset();
        const auto rightDataset = GetSelectedRightDataset();

        for (auto& row : m_byColumnRows)
            {
            // left column choices
            const wxString currentLeft = row.m_leftColumnChoice->GetStringSelection();
            row.m_leftColumnChoice->Clear();
            if (leftDataset != nullptr)
                {
                if (leftDataset->HasValidIdData())
                    {
                    row.m_leftColumnChoice->Append(leftDataset->GetIdColumn().GetName());
                    }
                for (const auto& name : leftDataset->GetCategoricalColumnNames())
                    {
                    row.m_leftColumnChoice->Append(name);
                    }
                }
            if (!currentLeft.empty())
                {
                const int idx = row.m_leftColumnChoice->FindString(currentLeft);
                if (idx != wxNOT_FOUND)
                    {
                    row.m_leftColumnChoice->SetSelection(idx);
                    }
                }

            // right column choices
            const wxString currentRight = row.m_rightColumnChoice->GetStringSelection();
            row.m_rightColumnChoice->Clear();
            if (rightDataset != nullptr)
                {
                if (rightDataset->HasValidIdData())
                    {
                    row.m_rightColumnChoice->Append(rightDataset->GetIdColumn().GetName());
                    }
                for (const auto& name : rightDataset->GetCategoricalColumnNames())
                    {
                    row.m_rightColumnChoice->Append(name);
                    }
                }
            if (!currentRight.empty())
                {
                const int idx = row.m_rightColumnChoice->FindString(currentRight);
                if (idx != wxNOT_FOUND)
                    {
                    row.m_rightColumnChoice->SetSelection(idx);
                    }
                }
            }
        }

    //-------------------------------------------
    void JoinDlg::ResetPreviewGrid()
        {
        m_previewGrid->SetTable(new wxGridStringTable(0, 0), true);
        m_previewGrid->ForceRefresh();
        }

    //-------------------------------------------
    void JoinDlg::UpdatePreview()
        {
        m_joinedDataset.reset();

        const auto leftDataset = GetSelectedLeftDataset();
        const auto rightDataset = GetSelectedRightDataset();
        if (leftDataset == nullptr || rightDataset == nullptr)
            {
            ResetPreviewGrid();
            return;
            }

        // collect by-column pairs
        std::vector<std::pair<wxString, wxString>> byColumns;
        for (const auto& row : m_byColumnRows)
            {
            const wxString leftCol = row.m_leftColumnChoice->GetStringSelection();
            const wxString rightCol = row.m_rightColumnChoice->GetStringSelection();
            if (!leftCol.empty() && !rightCol.empty())
                {
                byColumns.emplace_back(leftCol, rightCol);
                }
            }

        if (byColumns.empty())
            {
            ResetPreviewGrid();
            return;
            }

        const wxString suffix = m_suffixCtrl->GetValue();
        const int joinTypeSel = m_joinTypeChoice->GetSelection();

        try
            {
            switch (joinTypeSel)
                {
            case 0: // left join unique last
                m_joinedDataset = Data::DatasetLeftJoin::LeftJoinUniqueLast(
                    leftDataset, rightDataset, byColumns, suffix);
                break;
            case 1: // left join unique first
                m_joinedDataset = Data::DatasetLeftJoin::LeftJoinUniqueFirst(
                    leftDataset, rightDataset, byColumns, suffix);
                break;
            case 2: // left join
                m_joinedDataset =
                    Data::DatasetLeftJoin::LeftJoin(leftDataset, rightDataset, byColumns, suffix);
                break;
            case 3: // inner join
                m_joinedDataset =
                    Data::DatasetInnerJoin::InnerJoin(leftDataset, rightDataset, byColumns, suffix);
                break;
            default:
                ResetPreviewGrid();
                return;
                }
            }
        catch (const std::exception& exc)
            {
            wxLogDebug(L"Join preview failed: %s", wxString::FromUTF8(exc.what()));
            ResetPreviewGrid();
            return;
            }
        catch (...)
            {
            ResetPreviewGrid();
            return;
            }

        if (m_joinedDataset == nullptr)
            {
            ResetPreviewGrid();
            return;
            }

        auto* table = new DatasetGridTable(m_joinedDataset);
        table->SetMaxRows(Settings::PREVIEW_MAX_ROWS);
        m_previewGrid->SetTable(table, true);
        m_previewGrid->AutoSizeColumns(false);
        m_previewGrid->ForceRefresh();
        }

    //-------------------------------------------
    void JoinDlg::SetDefaultOutputName()
        {
        const wxString leftName = GetSelectedLeftDatasetName();
        const wxString rightName = GetSelectedRightDatasetName();
        if (leftName.empty() || rightName.empty())
            {
            m_outputNameCtrl->SetValue(wxString{});
            return;
            }
        const wxString baseName = wxString::Format(_(L"%s + %s (Joined)"), leftName, rightName);
        m_outputNameCtrl->SetValue((m_reportBuilder != nullptr) ?
                                       m_reportBuilder->GenerateUniqueDatasetName(baseName) :
                                       baseName);
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> JoinDlg::GetSelectedLeftDataset() const
        {
        if (m_reportBuilder == nullptr || m_leftDatasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_leftDatasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return nullptr;
            }

        const auto& datasets = m_reportBuilder->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    wxString JoinDlg::GetSelectedLeftDatasetName() const
        {
        const int sel = m_leftDatasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return {};
            }
        return m_datasetNames[sel];
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> JoinDlg::GetSelectedRightDataset() const
        {
        if (m_reportBuilder == nullptr || m_rightDatasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_rightDatasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return nullptr;
            }

        const auto& datasets = m_reportBuilder->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    wxString JoinDlg::GetSelectedRightDatasetName() const
        {
        const int sel = m_rightDatasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return {};
            }
        return m_datasetNames[sel];
        }

    //-------------------------------------------
    wxString JoinDlg::GetOutputName() const
        {
        return m_outputNameCtrl->GetValue().Strip(wxString::both);
        }

    //-------------------------------------------
    JoinOptions JoinDlg::GetJoinOptions() const
        {
        JoinOptions opts;
        opts.m_sourceDatasetName = GetSelectedLeftDatasetName();
        opts.m_otherDatasetName = GetSelectedRightDatasetName();
        opts.m_outputName = GetOutputName();
        opts.m_suffix = m_suffixCtrl->GetValue();

        switch (m_joinTypeChoice->GetSelection())
            {
        case 0:
            opts.m_type = JoinOptions::JoinType::LeftJoinUniqueLast;
            break;
        case 1:
            opts.m_type = JoinOptions::JoinType::LeftJoinUniqueFirst;
            break;
        case 2:
            opts.m_type = JoinOptions::JoinType::LeftJoin;
            break;
        case 3:
            opts.m_type = JoinOptions::JoinType::InnerJoin;
            break;
        default:
            opts.m_type = JoinOptions::JoinType::LeftJoinUniqueLast;
            break;
            }

        for (const auto& row : m_byColumnRows)
            {
            const wxString leftCol = row.m_leftColumnChoice->GetStringSelection();
            const wxString rightCol = row.m_rightColumnChoice->GetStringSelection();
            if (!leftCol.empty() && !rightCol.empty())
                {
                opts.m_byColumns.emplace_back(leftCol, rightCol);
                }
            }

        return opts;
        }

    //-------------------------------------------
    bool JoinDlg::Validate()
        {
        if (GetSelectedLeftDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a left dataset."), _(L"No Dataset"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        if (GetSelectedRightDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a right dataset."), _(L"No Dataset"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        // verify at least one complete by-column pair
        bool hasByColumn{ false };
        for (const auto& row : m_byColumnRows)
            {
            if (!row.m_leftColumnChoice->GetStringSelection().empty() &&
                !row.m_rightColumnChoice->GetStringSelection().empty())
                {
                hasByColumn = true;
                break;
                }
            }
        if (!hasByColumn)
            {
            wxMessageBox(_(L"Please specify at least one column pair to join by."),
                         _(L"Column Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
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

        if (m_joinedDataset == nullptr)
            {
            wxMessageBox(_(L"The join could not be generated. "
                           "Please verify your column selections."),
                         _(L"Join Error"), wxOK | wxICON_WARNING, this);
            return false;
            }

        return true;
        }
    } // namespace Wisteria::UI
