///////////////////////////////////////////////////////////////////////////////
// Name:        pivotwiderrdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pivotwiderrdlg.h"

namespace Wisteria::UI
    {
    [[nodiscard]]
    static wxString NaNLabel()
        {
        return L"NaN";
        }

    //-------------------------------------------
    PivotWiderDlg::PivotWiderDlg(const ReportBuilder* reportBuilder, wxWindow* parent,
                                 const wxWindowID id, const wxPoint& pos, const wxSize& size,
                                 const long style)
        : wxDialog(parent, id, _(L"Pivot Wider"), pos, size, style), m_reportBuilder(reportBuilder)
        {
        CreateControls();

        SetMinSize(FromDIP(wxSize{ 500, 550 }));
        SetSize(FromDIP(wxSize{ 550, 800 }));
        Centre();

        SetDefaultOutputName();
        }

    //-------------------------------------------
    void PivotWiderDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        datasetSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Dataset:")),
                          wxSizerFlags{}.CenterVertical());
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
        datasetSizer->Add(m_datasetChoice, wxSizerFlags{}.Expand());
        mainSizer->Add(datasetSizer, wxSizerFlags{}.Expand().Border());

        // column selection button
        auto* colButton = new wxButton(this, wxID_ANY, _(L"Select Columns..."));
        mainSizer->Add(colButton, wxSizerFlags{}.Border(wxLEFT));

        // column labels
        const wxColour varLabelColor{ 0, 102, 204 };
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* idLabel = new wxStaticText(this, wxID_ANY, _(L"ID columns:"));
        idLabel->SetFont(idLabel->GetFont().Bold());
        varGrid->Add(idLabel, wxSizerFlags{}.CenterVertical());
        m_idColumnsLabel = new wxStaticText(this, wxID_ANY, wxString{});
        m_idColumnsLabel->SetForegroundColour(varLabelColor);
        varGrid->Add(m_idColumnsLabel, wxSizerFlags{}.CenterVertical());

        auto* namesLabel = new wxStaticText(this, wxID_ANY, _(L"Names from:"));
        namesLabel->SetFont(namesLabel->GetFont().Bold());
        varGrid->Add(namesLabel, wxSizerFlags{}.CenterVertical());
        m_namesFromLabel = new wxStaticText(this, wxID_ANY, wxString{});
        m_namesFromLabel->SetForegroundColour(varLabelColor);
        varGrid->Add(m_namesFromLabel, wxSizerFlags{}.CenterVertical());

        auto* valuesLabel = new wxStaticText(this, wxID_ANY, _(L"Values from:"));
        valuesLabel->SetFont(valuesLabel->GetFont().Bold());
        varGrid->Add(valuesLabel, wxSizerFlags{}.CenterVertical());
        m_valuesFromLabel = new wxStaticText(this, wxID_ANY, wxString{});
        m_valuesFromLabel->SetForegroundColour(varLabelColor);
        varGrid->Add(m_valuesFromLabel, wxSizerFlags{}.CenterVertical());

        mainSizer->Add(varGrid, wxSizerFlags{}.Expand().Border());

        // options
        auto* optionsBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Options"));
        auto* optGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        optGrid->AddGrowableCol(1, 1);

        optGrid->Add(new wxStaticText(optionsBox->GetStaticBox(), wxID_ANY, _(L"Names separator:")),
                     wxSizerFlags{}.CenterVertical());
        m_namesSepCtrl = new wxTextCtrl(optionsBox->GetStaticBox(), wxID_ANY, L"_");
        optGrid->Add(m_namesSepCtrl, wxSizerFlags{}.Expand());

        optGrid->Add(new wxStaticText(optionsBox->GetStaticBox(), wxID_ANY, _(L"Names prefix:")),
                     wxSizerFlags{}.CenterVertical());
        m_namesPrefixCtrl = new wxTextCtrl(optionsBox->GetStaticBox(), wxID_ANY, wxString{});
        optGrid->Add(m_namesPrefixCtrl, wxSizerFlags{}.Expand());

        optGrid->Add(new wxStaticText(optionsBox->GetStaticBox(), wxID_ANY, _(L"Fill value:")),
                     wxSizerFlags{}.CenterVertical());
        m_fillValueCtrl = new wxTextCtrl(optionsBox->GetStaticBox(), wxID_ANY, NaNLabel());
        optGrid->Add(m_fillValueCtrl, wxSizerFlags{}.Expand());

        optionsBox->Add(optGrid, wxSizerFlags{}.Expand().Border());
        mainSizer->Add(optionsBox, wxSizerFlags{}.Expand().Border());

        // output name
        auto* nameSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        nameSizer->AddGrowableCol(1, 1);
        nameSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Output name:")),
                       wxSizerFlags{}.CenterVertical());
        m_outputNameCtrl = new wxTextCtrl(this, wxID_ANY, wxString{});
        nameSizer->Add(m_outputNameCtrl, wxSizerFlags{}.Expand());
        mainSizer->Add(nameSizer, wxSizerFlags{}.Expand().Border());

        // preview grid
        auto* previewBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_previewGrid = new wxGrid(previewBox->GetStaticBox(), wxID_ANY);
        m_previewGrid->CreateGrid(0, 0);
        m_previewGrid->EnableEditing(false);
        m_previewGrid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
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

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        colButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectColumns(); });
        }

    //-------------------------------------------
    void PivotWiderDlg::OnDatasetChanged()
        {
        m_idColumns.clear();
        m_namesFromColumn.clear();
        m_valuesFromColumns.clear();
        m_pivotedDataset.reset();
        UpdateColumnLabels();
        SetDefaultOutputName();
        UpdatePreview();
        }

    //-------------------------------------------
    void PivotWiderDlg::OnSelectColumns()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        Data::Dataset::ColumnPreviewInfo columnInfo;
        if (m_reportBuilder != nullptr)
            {
            const auto& importOpts = m_reportBuilder->GetDatasetImportOptions();
            const auto dsName = GetSelectedDatasetName();
            const auto it = importOpts.find(dsName);
            if (it != importOpts.cend())
                {
                columnInfo = it->second.m_columnPreviewInfo;
                }
            }
        if (columnInfo.empty())
            {
            columnInfo = BuildColumnPreviewInfo(*dataset);
            }

        using VLI = VariableSelectDlg::VariableListInfo;
        VariableSelectDlg dlg(this, columnInfo,
                              { VLI{}.Label(_(L"ID Columns")),
                                VLI{}.Label(_(L"Names From (categorical)")).SingleSelection(true),
                                VLI{}.Label(_(L"Values From (continuous)")).Required(false) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_idColumns = dlg.GetSelectedVariables(0);
        const auto namesVars = dlg.GetSelectedVariables(1);
        m_namesFromColumn = namesVars.empty() ? wxString{} : namesVars.front();
        m_valuesFromColumns = dlg.GetSelectedVariables(2);

        UpdateColumnLabels();
        UpdatePreview();
        }

    //-------------------------------------------
    void PivotWiderDlg::UpdateColumnLabels()
        {
        wxString idStr;
        for (const auto& col : m_idColumns)
            {
            if (!idStr.empty())
                {
                idStr += L", ";
                }
            idStr += col;
            }
        m_idColumnsLabel->SetLabel(idStr);
        m_namesFromLabel->SetLabel(m_namesFromColumn);

        wxString valStr;
        for (const auto& col : m_valuesFromColumns)
            {
            if (!valStr.empty())
                {
                valStr += L", ";
                }
            valStr += col;
            }
        m_valuesFromLabel->SetLabel(valStr);

        Layout();
        }

    //-------------------------------------------
    void PivotWiderDlg::UpdatePreview()
        {
        // clear existing grid
        if (m_previewGrid->GetNumberRows() > 0)
            {
            m_previewGrid->DeleteRows(0, m_previewGrid->GetNumberRows());
            }
        if (m_previewGrid->GetNumberCols() > 0)
            {
            m_previewGrid->DeleteCols(0, m_previewGrid->GetNumberCols());
            }

        m_pivotedDataset.reset();

        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr || m_idColumns.empty() || m_namesFromColumn.empty())
            {
            return;
            }

        const double fillValue = [this]()
        {
            double val{ std::numeric_limits<double>::quiet_NaN() };
            const wxString fillStr = m_fillValueCtrl->GetValue().Strip(wxString::both);
            if (!fillStr.empty() && fillStr.CmpNoCase(NaNLabel()) != 0)
                {
                fillStr.ToDouble(&val);
                }
            return val;
        }();

        try
            {
            m_pivotedDataset = Data::Pivot::PivotWider(
                dataset, m_idColumns, m_namesFromColumn, m_valuesFromColumns,
                m_namesSepCtrl->GetValue(), m_namesPrefixCtrl->GetValue(), fillValue);
            }
        catch (...)
            {
            return;
            }

        if (m_pivotedDataset == nullptr)
            {
            return;
            }

        // fill preview grid
        auto* table = new DatasetGridTable(m_pivotedDataset);
        table->SetMaxRows(Settings::PREVIEW_MAX_ROWS);
        m_previewGrid->SetTable(table, true);
        m_previewGrid->AutoSizeColumns(false);
        m_previewGrid->ForceRefresh();
        }

    //-------------------------------------------
    void PivotWiderDlg::SetDefaultOutputName()
        {
        const wxString dsName = GetSelectedDatasetName();
        if (dsName.empty())
            {
            m_outputNameCtrl->SetValue(wxString{});
            return;
            }
        const wxString baseName = wxString::Format(_(L"%s (Pivoted Wider)"), dsName);
        m_outputNameCtrl->SetValue((m_reportBuilder != nullptr) ?
                                       m_reportBuilder->GenerateUniqueDatasetName(baseName) :
                                       baseName);
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    PivotWiderDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
        {
        Data::Dataset::ColumnPreviewInfo info;

        if (dataset.HasValidIdData())
            {
            info.push_back({ dataset.GetIdColumn().GetName(),
                             Data::Dataset::ColumnImportType::String, wxString{} });
            }
        for (const auto& col : dataset.GetCategoricalColumns())
            {
            info.push_back({ col.GetName(), Data::Dataset::ColumnImportType::String, wxString{} });
            }
        for (const auto& col : dataset.GetDateColumns())
            {
            info.push_back({ col.GetName(), Data::Dataset::ColumnImportType::Date, wxString{} });
            }
        for (const auto& col : dataset.GetContinuousColumns())
            {
            info.push_back({ col.GetName(), Data::Dataset::ColumnImportType::Numeric, wxString{} });
            }

        return info;
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> PivotWiderDlg::GetSelectedDataset() const
        {
        if (m_reportBuilder == nullptr || m_datasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || static_cast<size_t>(sel) >= m_datasetNames.size())
            {
            return nullptr;
            }

        const auto& datasets = m_reportBuilder->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    wxString PivotWiderDlg::GetSelectedDatasetName() const
        {
        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || static_cast<size_t>(sel) >= m_datasetNames.size())
            {
            return {};
            }
        return m_datasetNames[sel];
        }

    //-------------------------------------------
    wxString PivotWiderDlg::GetOutputName() const
        {
        return m_outputNameCtrl->GetValue().Strip(wxString::both);
        }

    //-------------------------------------------
    PivotWiderOptions PivotWiderDlg::GetPivotOptions() const
        {
        PivotWiderOptions opts;
        opts.m_sourceDatasetName = GetSelectedDatasetName();
        opts.m_outputName = GetOutputName();
        opts.m_idColumns = m_idColumns;
        opts.m_namesFromColumn = m_namesFromColumn;
        opts.m_valuesFromColumns = m_valuesFromColumns;
        opts.m_namesSep = m_namesSepCtrl->GetValue();
        opts.m_namesPrefix = m_namesPrefixCtrl->GetValue();

        const wxString fillStr = m_fillValueCtrl->GetValue().Strip(wxString::both);
        if (!fillStr.empty() && fillStr.CmpNoCase(NaNLabel()) != 0)
            {
            fillStr.ToDouble(&opts.m_fillValue);
            }

        return opts;
        }

    //-------------------------------------------
    bool PivotWiderDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_idColumns.empty())
            {
            wxMessageBox(_(L"Please select at least one ID column."), _(L"Column Not Specified"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        if (m_namesFromColumn.empty())
            {
            wxMessageBox(_(L"Please select a \"names from\" column."), _(L"Column Not Specified"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        if (GetOutputName().empty())
            {
            wxMessageBox(_(L"Please specify an output dataset name."), _(L"Name Required"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }

        if (m_reportBuilder != nullptr && m_reportBuilder->GetDatasets().contains(GetOutputName()))
            {
            wxMessageBox(_(L"A dataset with this name already exists. "
                           "Please choose a different output name."),
                         _(L"Duplicate Name"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (m_pivotedDataset == nullptr)
            {
            wxMessageBox(_(L"The pivot could not be generated. "
                           "Please verify your column selections."),
                         _(L"Pivot Error"), wxOK | wxICON_WARNING, this);
            return false;
            }

        return true;
        }
    } // namespace Wisteria::UI
