///////////////////////////////////////////////////////////////////////////////
// Name:        pivotlongerdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pivotlongerdlg.h"
#include <wx/tokenzr.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    PivotLongerDlg::PivotLongerDlg(const ReportBuilder* reportBuilder, wxWindow* parent,
                                   const wxWindowID id, const wxPoint& pos, const wxSize& size,
                                   const long style)
        : wxDialog(parent, id, _(L"Pivot Longer"), pos, size, style), m_reportBuilder(reportBuilder)
        {
        CreateControls();

        SetMinSize(FromDIP(wxSize{ 500, 550 }));
        SetSize(FromDIP(wxSize{ 550, 600 }));
        Centre();

        SetDefaultOutputName();
        }

    //-------------------------------------------
    void PivotLongerDlg::CreateControls()
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

        auto* keepLabel = new wxStaticText(this, wxID_ANY, _(L"Columns to keep:"));
        keepLabel->SetFont(keepLabel->GetFont().Bold());
        varGrid->Add(keepLabel, wxSizerFlags{}.CenterVertical());
        m_keepColumnsLabel = new wxStaticText(this, wxID_ANY, wxString{});
        m_keepColumnsLabel->SetForegroundColour(varLabelColor);
        varGrid->Add(m_keepColumnsLabel, wxSizerFlags{}.CenterVertical());

        auto* fromLabel = new wxStaticText(this, wxID_ANY, _(L"Columns to pivot:"));
        fromLabel->SetFont(fromLabel->GetFont().Bold());
        varGrid->Add(fromLabel, wxSizerFlags{}.CenterVertical());
        m_fromColumnsLabel = new wxStaticText(this, wxID_ANY, wxString{});
        m_fromColumnsLabel->SetForegroundColour(varLabelColor);
        varGrid->Add(m_fromColumnsLabel, wxSizerFlags{}.CenterVertical());

        mainSizer->Add(varGrid, wxSizerFlags{}.Expand().Border());

        // target column names
        auto* targetBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Target Columns"));
        auto* targetGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        targetGrid->AddGrowableCol(1, 1);

        targetGrid->Add(new wxStaticText(targetBox->GetStaticBox(), wxID_ANY, _(L"Names to:")),
                        wxSizerFlags{}.CenterVertical());
        m_namesToCtrl = new wxTextCtrl(targetBox->GetStaticBox(), wxID_ANY, DefaultNamesTo());
        m_namesToCtrl->SetToolTip(
            _(L"Names for the new grouping columns, comma-separated if multiple."));
        targetGrid->Add(m_namesToCtrl, wxSizerFlags{}.Expand());

        targetGrid->Add(new wxStaticText(targetBox->GetStaticBox(), wxID_ANY, _(L"Values to:")),
                        wxSizerFlags{}.CenterVertical());
        m_valuesToCtrl = new wxTextCtrl(targetBox->GetStaticBox(), wxID_ANY, DefaultValuesTo());
        m_valuesToCtrl->SetToolTip(_(L"Name for the new value column."));
        targetGrid->Add(m_valuesToCtrl, wxSizerFlags{}.Expand());

        targetGrid->Add(new wxStaticText(targetBox->GetStaticBox(), wxID_ANY, _(L"Names pattern:")),
                        wxSizerFlags{}.CenterVertical());
        m_namesPatternCtrl = new wxTextCtrl(targetBox->GetStaticBox(), wxID_ANY, wxString{});
        m_namesPatternCtrl->SetToolTip(
            _(L"Optional regex with capture groups to split column names "
              "into multiple target columns. Leave blank to use full names."));
        targetGrid->Add(m_namesPatternCtrl, wxSizerFlags{}.Expand());

        targetBox->Add(targetGrid, wxSizerFlags{}.Expand().Border());
        mainSizer->Add(targetBox, wxSizerFlags{}.Expand().Border());

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
    void PivotLongerDlg::OnDatasetChanged()
        {
        m_columnsToKeep.clear();
        m_fromColumns.clear();
        m_pivotedDataset.reset();
        UpdateColumnLabels();
        SetDefaultOutputName();
        UpdatePreview();
        }

    //-------------------------------------------
    void PivotLongerDlg::OnSelectColumns()
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
        VariableSelectDlg dlg(
            this, columnInfo,
            { VLI{}
                  .Label(_(L"Columns to Keep (ID/grouping)"))
                  .Required(true)
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Columns to Pivot"))
                  .Required(true)
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_columnsToKeep = dlg.GetSelectedVariables(0);
        m_fromColumns = dlg.GetSelectedVariables(1);

        UpdateColumnLabels();
        UpdatePreview();
        }

    //-------------------------------------------
    void PivotLongerDlg::UpdateColumnLabels()
        {
        wxString keepStr;
        for (const auto& col : m_columnsToKeep)
            {
            if (!keepStr.empty())
                {
                keepStr += L", ";
                }
            keepStr += col;
            }
        m_keepColumnsLabel->SetLabel(keepStr);

        wxString fromStr;
        for (const auto& col : m_fromColumns)
            {
            if (!fromStr.empty())
                {
                fromStr += L", ";
                }
            fromStr += col;
            }
        m_fromColumnsLabel->SetLabel(fromStr);

        Layout();
        }

    //-------------------------------------------
    void PivotLongerDlg::UpdatePreview()
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
        if (dataset == nullptr || m_columnsToKeep.empty() || m_fromColumns.empty())
            {
            return;
            }

        // parse names-to as comma-separated list
        std::vector<wxString> namesTo;
        const wxString namesToStr = m_namesToCtrl->GetValue().Strip(wxString::both);
        if (!namesToStr.empty())
            {
            wxStringTokenizer tokenizer(namesToStr, L",");
            while (tokenizer.HasMoreTokens())
                {
                const wxString token = tokenizer.GetNextToken().Strip(wxString::both);
                if (!token.empty())
                    {
                    namesTo.push_back(token);
                    }
                }
            }
        if (namesTo.empty())
            {
            namesTo.push_back(DefaultNamesTo());
            }

        const wxString valuesTo = m_valuesToCtrl->GetValue().Strip(wxString::both).empty() ?
                                      DefaultValuesTo() :
                                      m_valuesToCtrl->GetValue().Strip(wxString::both);

        try
            {
            m_pivotedDataset =
                Data::Pivot::PivotLonger(dataset, m_columnsToKeep, m_fromColumns, namesTo, valuesTo,
                                         m_namesPatternCtrl->GetValue());
            }
        catch (...)
            {
            return;
            }

        if (m_pivotedDataset == nullptr)
            {
            return;
            }

        // fill preview grid (up to 50 rows)
        auto* table = new DatasetGridTable(m_pivotedDataset);
        table->SetMaxRows(Settings::PREVIEW_MAX_ROWS);
        m_previewGrid->SetTable(table, true);
        m_previewGrid->AutoSizeColumns(false);
        m_previewGrid->ForceRefresh();
        }

    //-------------------------------------------
    void PivotLongerDlg::SetDefaultOutputName()
        {
        const wxString dsName = GetSelectedDatasetName();
        if (dsName.empty())
            {
            m_outputNameCtrl->SetValue(wxString{});
            return;
            }
        const wxString baseName = wxString::Format(_(L"%s (Pivoted Longer)"), dsName);
        m_outputNameCtrl->SetValue((m_reportBuilder != nullptr) ?
                                       m_reportBuilder->GenerateUniqueDatasetName(baseName) :
                                       baseName);
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    PivotLongerDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> PivotLongerDlg::GetSelectedDataset() const
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
    wxString PivotLongerDlg::GetSelectedDatasetName() const
        {
        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return {};
            }
        return m_datasetNames[sel];
        }

    //-------------------------------------------
    wxString PivotLongerDlg::GetOutputName() const
        {
        return m_outputNameCtrl->GetValue().Strip(wxString::both);
        }

    //-------------------------------------------
    PivotLongerOptions PivotLongerDlg::GetPivotOptions() const
        {
        PivotLongerOptions opts;
        opts.m_sourceDatasetName = GetSelectedDatasetName();
        opts.m_outputName = GetOutputName();
        opts.m_columnsToKeep = m_columnsToKeep;
        opts.m_fromColumns = m_fromColumns;
        opts.m_valuesTo = m_valuesToCtrl->GetValue().Strip(wxString::both);
        opts.m_namesPattern = m_namesPatternCtrl->GetValue();

        // parse names-to
        const wxString namesToStr = m_namesToCtrl->GetValue().Strip(wxString::both);
        if (!namesToStr.empty())
            {
            wxStringTokenizer tokenizer(namesToStr, L",");
            while (tokenizer.HasMoreTokens())
                {
                const wxString token = tokenizer.GetNextToken().Strip(wxString::both);
                if (!token.empty())
                    {
                    opts.m_namesTo.push_back(token);
                    }
                }
            }

        return opts;
        }

    //-------------------------------------------
    bool PivotLongerDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_columnsToKeep.empty())
            {
            wxMessageBox(_(L"Please select at least one column to keep."),
                         _(L"Column Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (m_fromColumns.empty())
            {
            wxMessageBox(_(L"Please select at least one column to pivot."),
                         _(L"Column Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (m_valuesToCtrl->GetValue().Strip(wxString::both).empty())
            {
            wxMessageBox(_(L"Please specify a \"values to\" column name."), _(L"Name Required"),
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
