///////////////////////////////////////////////////////////////////////////////
// Name:        datasetimportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "datasetimportdlg.h"
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //----------------------------------------------
    DatasetImportDlg::DatasetImportDlg(wxWindow* parent, const wxString& filePath, wxWindowID id,
                                       const wxString& caption, const wxPoint& pos,
                                       const wxSize& size, long style)
        : m_filePath(filePath), m_fileExt(wxFileName(filePath).GetExt())
        {
        wxWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style);

        // load worksheet names for spreadsheet files
        if (m_fileExt.CmpNoCase(L"xlsx") == 0)
            {
            Data::ExcelReader xlReader(m_filePath);
            m_worksheetNames = xlReader.GetWorksheetNames();
            }
        else if (m_fileExt.CmpNoCase(L"ods") == 0)
            {
            Data::OdsReader odsReader(m_filePath);
            m_worksheetNames = odsReader.GetWorksheetNames();
            }

        CreateControls();
        RefreshPreview();

        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //----------------------------------------------
    void DatasetImportDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        // options section
        auto* optionsSizer = new wxFlexGridSizer(2, FromDIP(5), FromDIP(10));
        optionsSizer->AddGrowableCol(1, 1);

        const bool isSpreadsheet =
            (m_fileExt.CmpNoCase(L"xlsx") == 0 || m_fileExt.CmpNoCase(L"ods") == 0);

        // worksheet selector (spreadsheets only)
        m_worksheetLabel = new wxStaticText(this, wxID_ANY, _(L"Worksheet:"));
        optionsSizer->Add(m_worksheetLabel, wxSizerFlags{}.CenterVertical());

        m_worksheetChoice = new wxChoice(this, wxID_ANY);
        for (const auto& name : m_worksheetNames)
            {
            m_worksheetChoice->Append(name);
            }
        if (!m_worksheetNames.empty())
            {
            m_worksheetChoice->SetSelection(0);
            }
        optionsSizer->Add(m_worksheetChoice, wxSizerFlags{});

        m_worksheetLabel->Show(isSpreadsheet);
        m_worksheetChoice->Show(isSpreadsheet);

        // skip rows
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Skip rows:")),
                          wxSizerFlags{}.CenterVertical());
        const auto spinSize = FromDIP(wxSize{ 90, -1 });
        m_skipRowsSpin = new wxSpinCtrl(this, wxID_ANY, L"0", wxDefaultPosition, spinSize,
                                        wxSP_ARROW_KEYS, 0, 1000, 0);
        optionsSizer->Add(m_skipRowsSpin, wxSizerFlags{});

        // max discrete value
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Max discrete value:")),
                          wxSizerFlags{}.CenterVertical());
        m_maxDiscreteSpin = new wxSpinCtrl(this, wxID_ANY, L"7", wxDefaultPosition, spinSize,
                                           wxSP_ARROW_KEYS, 1, 100, 7);
        optionsSizer->Add(m_maxDiscreteSpin, wxSizerFlags{});

        // ID column
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"ID column:")),
                          wxSizerFlags{}.CenterVertical());
        m_idColumnChoice = new wxChoice(this, wxID_ANY);
        m_idColumnChoice->Append(_(L"(None)"));
        m_idColumnChoice->SetSelection(0);
        optionsSizer->Add(m_idColumnChoice, wxSizerFlags{});

        mainSizer->Add(optionsSizer, wxSizerFlags{}.Expand().Border(wxALL, FromDIP(10)));

        // checkboxes
        auto* checkSizer = new wxBoxSizer(wxHORIZONTAL);

        m_leadingZerosCheck = new wxCheckBox(this, wxID_ANY, _(L"Leading zeros as text"));
        checkSizer->Add(m_leadingZerosCheck, wxSizerFlags{}.Border(wxRIGHT, FromDIP(15)));

        m_yearsAsTextCheck = new wxCheckBox(this, wxID_ANY, _(L"Years as text"));
        checkSizer->Add(m_yearsAsTextCheck);

        mainSizer->Add(checkSizer, wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10)));

        // preview grid
        mainSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Preview:")),
                       wxSizerFlags{}.Border(wxLEFT | wxBOTTOM, FromDIP(10)));

        m_previewGrid = new wxGrid(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize{ 1000, 400 }));
        m_previewGrid->SetDoubleBuffered(true);
        m_previewGrid->GetGridWindow()->SetDoubleBuffered(true);
        m_previewGrid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
        m_previewGrid->EnableEditing(false);
        mainSizer->Add(m_previewGrid,
                       wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxRIGHT, FromDIP(10)));

        // OK/Cancel
        mainSizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border(wxALL, FromDIP(10)));

        SetSizer(mainSizer);

        // bind events
        m_worksheetChoice->Bind(wxEVT_CHOICE, &DatasetImportDlg::OnOptionChanged, this);
        m_skipRowsSpin->Bind(wxEVT_SPINCTRL, &DatasetImportDlg::OnSpinChanged, this);
        m_maxDiscreteSpin->Bind(wxEVT_SPINCTRL, &DatasetImportDlg::OnSpinChanged, this);
        m_leadingZerosCheck->Bind(wxEVT_CHECKBOX, &DatasetImportDlg::OnOptionChanged, this);
        m_yearsAsTextCheck->Bind(wxEVT_CHECKBOX, &DatasetImportDlg::OnOptionChanged, this);
        m_idColumnChoice->Bind(wxEVT_CHOICE, &DatasetImportDlg::OnOptionChanged, this);
        }

    //----------------------------------------------
    void DatasetImportDlg::OnOptionChanged([[maybe_unused]] wxCommandEvent& event)
        {
        RefreshPreview();
        }

    //----------------------------------------------
    void DatasetImportDlg::OnSpinChanged([[maybe_unused]] wxSpinEvent& event) { RefreshPreview(); }

    //----------------------------------------------
    void DatasetImportDlg::RefreshPreview()
        {
        try
            {
            // build partial ImportInfo for column deduction
            Data::ImportInfo previewInfo;
            previewInfo.SkipRows(static_cast<size_t>(m_skipRowsSpin->GetValue()));
            previewInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscreteSpin->GetValue()));
            previewInfo.TreatLeadingZerosAsText(m_leadingZerosCheck->GetValue());
            previewInfo.TreatYearsAsText(m_yearsAsTextCheck->GetValue());

            const auto worksheet = GetWorksheet();

            // read column info from file
            const auto columnInfo = Data::Dataset::ReadColumnInfo(m_filePath, previewInfo,
                                                                  PREVIEW_ROW_COUNT, worksheet);

            // update the ID column choice with discovered column names
            const wxString previousId = (m_idColumnChoice->GetSelection() > 0) ?
                                            m_idColumnChoice->GetStringSelection() :
                                            wxString{};
            m_idColumnChoice->Clear();
            m_idColumnChoice->Append(_(L"(None)"));
            for (const auto& col : columnInfo)
                {
                m_idColumnChoice->Append(col.m_name);
                }
            if (!previousId.empty())
                {
                const int idx = m_idColumnChoice->FindString(previousId);
                m_idColumnChoice->SetSelection(idx != wxNOT_FOUND ? idx : 0);
                }
            else
                {
                m_idColumnChoice->SetSelection(0);
                }

            // convert to full ImportInfo
            auto importInfo = Data::Dataset::ImportInfoFromPreview(columnInfo);
            importInfo.SkipRows(static_cast<size_t>(m_skipRowsSpin->GetValue()));
            importInfo.TreatLeadingZerosAsText(m_leadingZerosCheck->GetValue());
            importInfo.TreatYearsAsText(m_yearsAsTextCheck->GetValue());
            importInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscreteSpin->GetValue()));
            if (m_idColumnChoice->GetSelection() > 0)
                {
                importInfo.IdColumn(m_idColumnChoice->GetStringSelection());
                }

            // import data for preview
            m_previewDataset = std::make_shared<Data::Dataset>();
            m_previewDataset->Import(m_filePath, importInfo, worksheet);

            // update grid
            auto* table = new DatasetGridTable(m_previewDataset, columnInfo);

            // apply currency symbols to continuous columns
            size_t contIdx{ 0 };
            for (const auto& col : columnInfo)
                {
                if (col.m_type == Data::Dataset::ColumnImportType::Numeric)
                    {
                    if (!col.m_currencySymbol.empty())
                        {
                        table->SetCurrencySymbol(contIdx, col.m_currencySymbol);
                        }
                    ++contIdx;
                    }
                }

            m_previewGrid->SetTable(table, true);
            ApplyColumnHeaderIcons(table);
            m_previewGrid->AutoSizeColumns(false);
            AdjustGridColumnsForIcons();
            m_previewGrid->ForceRefresh();
            }
        catch (const std::exception& exc)
            {
            // on error, show empty grid
            m_previewDataset = std::make_shared<Data::Dataset>();
            auto* table = new DatasetGridTable(m_previewDataset);
            m_previewGrid->SetTable(table, true);
            ApplyColumnHeaderIcons(table);
            m_previewGrid->AutoSizeColumns(false);
            AdjustGridColumnsForIcons();
            m_previewGrid->ForceRefresh();
            wxLogWarning(L"%s", wxString::FromUTF8(exc.what()));
            }
        }

    //----------------------------------------------
    void DatasetImportDlg::ApplyColumnHeaderIcons(DatasetGridTable* table)
        {
        const auto iconSize = wxSize{ m_previewGrid->FromDIP(16), m_previewGrid->FromDIP(16) };

        auto* attrProvider = new DatasetGridAttrProvider();
        for (int col = 0; col < table->GetNumberCols(); ++col)
            {
            wxString artId;
            switch (table->GetColumnType(col))
                {
            case DatasetGridColumnType::Id:
                artId = L"ID_CATEGORICAL";
                break;
            case DatasetGridColumnType::Categorical:
                artId = L"ID_CATEGORICAL";
                break;
            case DatasetGridColumnType::Date:
                artId = L"ID_DATE";
                break;
            case DatasetGridColumnType::Continuous:
                artId = L"ID_CONTINUOUS";
                break;
                }
            const auto bmp = wxArtProvider::GetBitmapBundle(artId).GetBitmap(iconSize);
            if (bmp.IsOk())
                {
                attrProvider->SetColumnHeaderRenderer(col, DatasetColumnHeaderRenderer(bmp));
                }
            }
        table->SetAttrProvider(attrProvider);
        }

    //----------------------------------------------
    void DatasetImportDlg::AdjustGridColumnsForIcons()
        {
        const int iconOffset = m_previewGrid->FromDIP(16) + 8;
        int maxColWidth = m_previewGrid->GetClientSize().GetWidth() / 4;
        if (maxColWidth <= 0)
            {
            maxColWidth = m_previewGrid->GetParent()->GetClientSize().GetWidth() / 4;
            }
        for (int col = 0; col < m_previewGrid->GetNumberCols(); ++col)
            {
            int newWidth = m_previewGrid->GetColSize(col) + iconOffset;
            if (maxColWidth > 0)
                {
                newWidth = std::min(newWidth, maxColWidth);
                }
            m_previewGrid->SetColSize(col, newWidth);
            }
        }

    //----------------------------------------------
    Data::Dataset::ColumnPreviewInfo DatasetImportDlg::GetColumnPreviewInfo() const
        {
        Data::ImportInfo scanInfo;
        scanInfo.SkipRows(static_cast<size_t>(m_skipRowsSpin->GetValue()));
        scanInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscreteSpin->GetValue()));
        scanInfo.TreatLeadingZerosAsText(m_leadingZerosCheck->GetValue());
        scanInfo.TreatYearsAsText(m_yearsAsTextCheck->GetValue());

        return Data::Dataset::ReadColumnInfo(m_filePath, scanInfo, std::nullopt, GetWorksheet());
        }

    //----------------------------------------------
    Data::ImportInfo DatasetImportDlg::GetImportInfo() const
        {
        const auto columnInfo = GetColumnPreviewInfo();

        auto importInfo = Data::Dataset::ImportInfoFromPreview(columnInfo);
        importInfo.SkipRows(static_cast<size_t>(m_skipRowsSpin->GetValue()));
        importInfo.TreatLeadingZerosAsText(m_leadingZerosCheck->GetValue());
        importInfo.TreatYearsAsText(m_yearsAsTextCheck->GetValue());
        importInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscreteSpin->GetValue()));

        if (m_idColumnChoice->GetSelection() > 0)
            {
            importInfo.IdColumn(m_idColumnChoice->GetStringSelection());
            }

        return importInfo;
        }

    //----------------------------------------------
    std::variant<wxString, size_t> DatasetImportDlg::GetWorksheet() const
        {
        if (!m_worksheetNames.empty() && m_worksheetChoice->GetSelection() != wxNOT_FOUND)
            {
            // 1-based index
            return static_cast<size_t>(m_worksheetChoice->GetSelection() + 1);
            }
        return static_cast<size_t>(1);
        }
    } // namespace Wisteria::UI
