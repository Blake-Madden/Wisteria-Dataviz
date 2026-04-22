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
        : m_filePath(filePath), m_fileExt(wxFileName{ filePath }.GetExt())
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
    DatasetImportDlg::DatasetImportDlg(wxWindow* parent, const wxString& filePath,
                                       const Data::ImportInfo& importInfo,
                                       const Data::Dataset::ColumnPreviewInfo& columnInfo,
                                       const std::variant<wxString, size_t>& worksheet,
                                       wxWindowID id, const wxString& caption, const wxPoint& pos,
                                       const wxSize& size, long style)
        : m_filePath(filePath), m_fileExt(wxFileName{ filePath }.GetExt()),
          m_skipRows(static_cast<int>(importInfo.GetSkipRows())),
          m_maxDiscrete(static_cast<int>(importInfo.GetMaxDiscreteValue())),
          m_leadingZeros(importInfo.GetTreatLeadingZerosAsText()),
          m_yearsAsText(importInfo.GetTreatYearsAsText()), m_columnInfo(columnInfo)
        {
        // build the MD values string from the codes
        if (importInfo.GetMDCodes().has_value())
            {
            for (const auto& code : importInfo.GetMDCodes().value())
                {
                if (!m_mdValues.empty())
                    {
                    m_mdValues += L", ";
                    }
                m_mdValues += code;
                }
            }

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

        // resolve worksheet selection to 0-based index
        if (const auto* wsIndex = std::get_if<size_t>(&worksheet))
            {
            m_worksheet = (*wsIndex > 0) ? static_cast<int>(*wsIndex - 1) : 0;
            }
        else if (const auto* wsName = std::get_if<wxString>(&worksheet))
            {
            for (size_t i = 0; i < m_worksheetNames.size(); ++i)
                {
                if (wsName->CmpNoCase(m_worksheetNames[i]) == 0)
                    {
                    m_worksheet = static_cast<int>(i);
                    break;
                    }
                }
            }

        CreateControls();
        TransferDataToWindow();

        // select the ID column from the import settings
        if (!importInfo.GetIdColumn().empty())
            {
            const int idx = m_idColumnChoice->FindString(importInfo.GetIdColumn());
            if (idx != wxNOT_FOUND)
                {
                m_idColumnChoice->SetSelection(idx);
                }
            }

        RefreshPreviewFromColumnInfo();

        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //----------------------------------------------
    void DatasetImportDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        // show filepath
        auto* filePathSizer = new wxBoxSizer(wxHORIZONTAL);
        filePathSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Filepath: ")),
                           wxSizerFlags{}.CenterVertical());
        auto* fileLabel = new wxStaticText(this, wxID_ANY, m_filePath);
        fileLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        filePathSizer->Add(fileLabel, wxSizerFlags{}.CenterVertical());
        mainSizer->Add(filePathSizer,
                       wxSizerFlags{}.Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder() * 2));

        // options section
        auto* optionsSizer = new wxFlexGridSizer{ 2, wxSizerFlags::GetDefaultBorder(),
                                                  wxSizerFlags::GetDefaultBorder() * 2 };
        optionsSizer->AddGrowableCol(1, 1);

        // worksheet selector (spreadsheets only)
        const bool isSpreadsheet =
            (m_fileExt.CmpNoCase(L"xlsx") == 0 || m_fileExt.CmpNoCase(L"ods") == 0);
        auto* worksheetLabel = new wxStaticText(this, wxID_ANY, _(L"Worksheet:"));
        optionsSizer->Add(worksheetLabel, wxSizerFlags{}.CenterVertical());

        auto* worksheetChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                             nullptr, 0, wxGenericValidator{ &m_worksheet });
        for (const auto& name : m_worksheetNames)
            {
            worksheetChoice->Append(name);
            }
        if (!m_worksheetNames.empty())
            {
            worksheetChoice->SetSelection(
                (m_worksheet >= 0 && std::cmp_less(m_worksheet, m_worksheetNames.size())) ?
                    m_worksheet :
                    0);
            }
        optionsSizer->Add(worksheetChoice, wxSizerFlags{});

        worksheetLabel->Show(isSpreadsheet);
        worksheetChoice->Show(isSpreadsheet);

        // skip rows
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Skip rows:")),
                          wxSizerFlags{}.CenterVertical());
        const auto spinSize = FromDIP(wxSize{ 90, -1 });
        auto* skipRowsSpin = new wxSpinCtrl(this, wxID_ANY, L"0", wxDefaultPosition, spinSize,
                                            wxSP_ARROW_KEYS, 0, 1000, m_skipRows);
        skipRowsSpin->SetValidator(wxGenericValidator{ &m_skipRows });
        optionsSizer->Add(skipRowsSpin, wxSizerFlags{});

        // max discrete value
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Max discrete value:")),
                          wxSizerFlags{}.CenterVertical());
        auto* maxDiscreteSpin = new wxSpinCtrl(this, wxID_ANY, L"7", wxDefaultPosition, spinSize,
                                               wxSP_ARROW_KEYS, 1, 100, m_maxDiscrete);
        maxDiscreteSpin->SetValidator(wxGenericValidator{ &m_maxDiscrete });
        optionsSizer->Add(maxDiscreteSpin, wxSizerFlags{});

        // checkboxes
        auto* leadingZerosCheck =
            new wxCheckBox(this, wxID_ANY, _(L"Leading zeros as text"), wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_leadingZeros });
        optionsSizer->Add(leadingZerosCheck, wxSizerFlags{}.CenterVertical());
        optionsSizer->AddStretchSpacer();

        auto* yearsAsTextCheck =
            new wxCheckBox(this, wxID_ANY, _(L"Years as text"), wxDefaultPosition, wxDefaultSize, 0,
                           wxGenericValidator{ &m_yearsAsText });
        optionsSizer->Add(yearsAsTextCheck, wxSizerFlags{}.CenterVertical());
        optionsSizer->AddStretchSpacer();

        // MD codes
        optionsSizer->Add(
            new wxStaticText(this, wxID_ANY, _(L"Missing data codes (comma separated):")),
            wxSizerFlags{}.CenterVertical());
        // populate with common MD codes only if not already set
        // (the editing constructor pre-populates m_mdValues)
        if (m_mdValues.empty())
            {
            for (const auto& code : Data::ImportInfo::GetCommonMDCodes())
                {
                if (!m_mdValues.empty())
                    {
                    m_mdValues += L", ";
                    }
                m_mdValues += code;
                }
            }
        auto* mdValuesText =
            new wxTextCtrl(this, wxID_ANY, m_mdValues, wxDefaultPosition,
                           wxSize{ FromDIP(300), -1 }, 0, wxGenericValidator{ &m_mdValues });
        optionsSizer->Add(mdValuesText, wxSizerFlags{ 1 }.CenterVertical());

        // ID column
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"ID column:")),
                          wxSizerFlags{}.CenterVertical());
        m_idColumnChoice = new wxChoice(this, wxID_ANY);
        m_idColumnChoice->Append(_(L"(None)"));
        m_idColumnChoice->SetSelection(0);
        m_idColumnChoice->Clear();
        m_idColumnChoice->Append(_(L"(None)"));
        for (const auto& col : m_columnInfo)
            {
            if (col.m_type == Data::Dataset::ColumnImportType::String)
                {
                m_idColumnChoice->Append(col.m_name);
                }
            }
        optionsSizer->Add(m_idColumnChoice, wxSizerFlags{});

        mainSizer->Add(optionsSizer,
                       wxSizerFlags{}.Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder() * 2));

        // column type controls
        auto* selectedColumnLabel = new wxStaticText(this, wxID_ANY, _(L"Column data type:"));
        optionsSizer->Add(selectedColumnLabel, wxSizerFlags{}.CenterVertical());
        m_columnTypeChoice = new wxChoice(this, wxID_ANY);
        m_columnTypeChoice->Append(_(L"Text"));
        m_columnTypeChoice->Append(_(L"Categorical"));
        m_columnTypeChoice->Append(_(L"Numeric"));
        m_columnTypeChoice->Append(_(L"Date"));
        m_columnTypeChoice->Append(_(L"Do not import"));
        m_columnTypeChoice->Disable();
        optionsSizer->Add(m_columnTypeChoice, wxSizerFlags{}.CenterVertical());

        // preview grid
        auto* previewBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_previewGrid = new wxGrid(previewBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                   FromDIP(wxSize{ 1000, 400 }));
        m_previewGrid->SetDoubleBuffered(true);
        m_previewGrid->GetGridWindow()->SetDoubleBuffered(true);
        m_previewGrid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
        m_previewGrid->EnableEditing(false);
        previewBox->Add(m_previewGrid, wxSizerFlags{ 1 }.Expand().Border());
        auto* previewNote =
            new wxStaticText(previewBox->GetStaticBox(), wxID_ANY,
                             wxString::Format(_(L"Preview is limited to the first %s rows."),
                                              wxNumberFormatter::ToString(
                                                  static_cast<long>(Settings::PREVIEW_MAX_ROWS))));
        previewNote->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
        previewBox->Add(previewNote, wxSizerFlags{}.Border(wxLEFT | wxBOTTOM));
        mainSizer->Add(previewBox, wxSizerFlags{ 1 }.Expand().Border());

        // OK/Cancel
        mainSizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder() * 2));

        SetSizer(mainSizer);

        // bind events
        worksheetChoice->Bind(wxEVT_CHOICE, &DatasetImportDlg::OnOptionChanged, this);
        skipRowsSpin->Bind(wxEVT_SPINCTRL, &DatasetImportDlg::OnSpinChanged, this);
        maxDiscreteSpin->Bind(wxEVT_SPINCTRL, &DatasetImportDlg::OnSpinChanged, this);
        leadingZerosCheck->Bind(wxEVT_CHECKBOX, &DatasetImportDlg::OnOptionChanged, this);
        yearsAsTextCheck->Bind(wxEVT_CHECKBOX, &DatasetImportDlg::OnOptionChanged, this);
        m_idColumnChoice->Bind(wxEVT_CHOICE, &DatasetImportDlg::OnOptionChanged, this);
        mdValuesText->Bind(wxEVT_TEXT, &DatasetImportDlg::OnOptionChanged, this);
        m_previewGrid->Bind(wxEVT_GRID_LABEL_LEFT_CLICK, &DatasetImportDlg::OnColumnHeaderClick,
                            this);
        m_previewGrid->Bind(wxEVT_GRID_SELECT_CELL, &DatasetImportDlg::OnColumnSelected, this);
        m_columnTypeChoice->Bind(wxEVT_CHOICE, &DatasetImportDlg::OnColumnTypeChanged, this);
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
        TransferDataFromWindow();
        try
            {
            // build partial ImportInfo for column deduction
            Data::ImportInfo previewInfo;
            previewInfo.SkipRows(static_cast<size_t>(m_skipRows));
            previewInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscrete));
            previewInfo.TreatLeadingZerosAsText(m_leadingZeros);
            previewInfo.TreatYearsAsText(m_yearsAsText);
            if (m_mdValues.empty())
                {
                previewInfo.MDCodes(std::nullopt);
                }
            else
                {
                std::vector<std::wstring> mdCodes;
                wxStringTokenizer mdTokenizer(m_mdValues, L" ,;", wxTOKEN_STRTOK);
                while (mdTokenizer.HasMoreTokens())
                    {
                    mdCodes.push_back(mdTokenizer.GetNextToken().ToStdWstring());
                    }
                previewInfo.MDCodes(mdCodes);
                }

            const auto worksheet = GetWorksheet();

            // read column info from file
            const auto freshColumnInfo = Data::Dataset::ReadColumnInfo(
                m_filePath, previewInfo, Settings::PREVIEW_MAX_ROWS, worksheet);

            // preserve user overrides (exclusion, type) from previous m_columnInfo
            const auto previousColumnInfo = std::move(m_columnInfo);
            m_columnInfo = freshColumnInfo;
            for (auto& col : m_columnInfo)
                {
                const auto prevIt =
                    std::ranges::find_if(previousColumnInfo, [&col](const auto& prev)
                                         { return prev.m_name.CmpNoCase(col.m_name) == 0; });
                if (prevIt != previousColumnInfo.cend())
                    {
                    col.m_excluded = prevIt->m_excluded;
                    if (prevIt->m_userOverridden)
                        {
                        // always preserve explicit user type overrides
                        col.m_type = prevIt->m_type;
                        col.m_userOverridden = true;
                        }
                    // for non-user-overridden types, let the fresh
                    // deduction win (e.g., when LeadingZeros changes)
                    }
                }

            // update the ID column choice with discovered column names
            const wxString previousId = (m_idColumnChoice->GetSelection() > 0) ?
                                            m_idColumnChoice->GetStringSelection() :
                                            wxString{};
            m_idColumnChoice->Clear();
            m_idColumnChoice->Append(_(L"(None)"));
            for (const auto& col : m_columnInfo)
                {
                if (col.m_type == Data::Dataset::ColumnImportType::String)
                    {
                    m_idColumnChoice->Append(col.m_name);
                    }
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

            UpdateGrid();
            }
        catch (const std::exception& exc)
            {
            // on error, show empty grid
            m_previewDataset = std::make_shared<Data::Dataset>();
            m_columnInfo.clear();
            auto* table = new DatasetGridTable(m_previewDataset);
            m_previewGrid->SetTable(table, true);
            m_previewGrid->SetSelectionMode(wxGrid::wxGridSelectColumns);
            ApplyColumnHeaderIcons(table);
            m_previewGrid->AutoSizeColumns(false);
            AdjustGridColumnsForIcons();
            m_previewGrid->ForceRefresh();
            wxLogWarning(L"%s", wxString::FromUTF8(exc.what()));
            }
        }

    //----------------------------------------------
    void DatasetImportDlg::RefreshPreviewFromColumnInfo()
        {
        try
            {
            RefreshPreview();
            }
        catch (const std::exception& exc)
            {
            wxLogWarning(L"%s", wxString::FromUTF8(exc.what()));
            }
        }

    //----------------------------------------------
    void DatasetImportDlg::UpdateGrid()
        {
        TransferDataFromWindow();
        // convert to full ImportInfo from current m_columnInfo
        auto importInfo = Data::Dataset::ImportInfoFromPreview(m_columnInfo);
        importInfo.SkipRows(static_cast<size_t>(m_skipRows));
        importInfo.TreatLeadingZerosAsText(m_leadingZeros);
        importInfo.TreatYearsAsText(m_yearsAsText);
        importInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscrete));
        if (m_mdValues.empty())
            {
            importInfo.MDCodes(std::nullopt);
            }
        else
            {
            std::vector<std::wstring> mdCodes;
            wxStringTokenizer mdTokenizer(m_mdValues, L" ,;", wxTOKEN_STRTOK);
            while (mdTokenizer.HasMoreTokens())
                {
                mdCodes.push_back(mdTokenizer.GetNextToken().ToStdWstring());
                }
            importInfo.MDCodes(mdCodes);
            }
        if (m_idColumnChoice->GetSelection() > 0)
            {
            importInfo.IdColumn(m_idColumnChoice->GetStringSelection());
            }

        const auto worksheet = GetWorksheet();

        // import data for preview
        m_previewDataset = std::make_shared<Data::Dataset>();
        m_previewDataset->Import(m_filePath, importInfo, worksheet);

        // update grid
        auto* table = new DatasetGridTable(m_previewDataset, m_columnInfo);
        table->SetMaxRows(Settings::PREVIEW_MAX_ROWS);

        // apply currency symbols to continuous columns
        size_t contIdx{ 0 };
        for (const auto& col : m_columnInfo)
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
        m_previewGrid->SetSelectionMode(wxGrid::wxGridSelectColumns);
        ApplyColumnHeaderIcons(table);
        m_previewGrid->AutoSizeColumns(false);
        AdjustGridColumnsForIcons();
        ApplyExcludedColumnStyling();
        m_previewGrid->ForceRefresh();
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
        const int iconOffset = m_previewGrid->FromDIP(24);
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
    void DatasetImportDlg::OnColumnHeaderClick(wxGridEvent& event)
        {
        const int col = event.GetCol();
        if (col < 0 || std::cmp_greater_equal(col, m_columnInfo.size()))
            {
            event.Skip();
            return;
            }

        m_previewGrid->SelectCol(col);
        UpdateColumnTypeControls();
        }

    //----------------------------------------------
    void DatasetImportDlg::OnColumnSelected(wxGridEvent& event)
        {
        event.Skip();
        // defer so the grid's selection state is updated first
        CallAfter([this]() { UpdateColumnTypeControls(); });
        }

    //----------------------------------------------
    void DatasetImportDlg::UpdateColumnTypeControls()
        {
        const auto selectedCols = m_previewGrid->GetSelectedCols();
        if (selectedCols.empty() || std::cmp_greater_equal(selectedCols[0], m_columnInfo.size()))
            {
            m_columnTypeChoice->SetSelection(wxNOT_FOUND);
            m_columnTypeChoice->Disable();
            return;
            }

        const auto col = static_cast<size_t>(selectedCols[0]);
        const auto& colInfo = m_columnInfo[col];

        m_columnTypeChoice->Enable();
        if (colInfo.m_excluded)
            {
            m_columnTypeChoice->SetSelection(4); // "Do not import"
            }
        else
            {
            switch (colInfo.m_type)
                {
            case Data::Dataset::ColumnImportType::String:
                [[fallthrough]];
            case Data::Dataset::ColumnImportType::DichotomousString:
                m_columnTypeChoice->SetSelection(0);
                break;
            case Data::Dataset::ColumnImportType::Discrete:
                [[fallthrough]];
            case Data::Dataset::ColumnImportType::DichotomousDiscrete:
                m_columnTypeChoice->SetSelection(1);
                break;
            case Data::Dataset::ColumnImportType::Numeric:
                m_columnTypeChoice->SetSelection(2);
                break;
            case Data::Dataset::ColumnImportType::Date:
                m_columnTypeChoice->SetSelection(3);
                break;
                }
            }
        }

    //----------------------------------------------
    void DatasetImportDlg::OnColumnTypeChanged([[maybe_unused]] wxCommandEvent& event)
        {
        const auto selectedCols = m_previewGrid->GetSelectedCols();
        if (selectedCols.empty())
            {
            return;
            }

        const int sel = m_columnTypeChoice->GetSelection();
        if (sel == wxNOT_FOUND)
            {
            return;
            }

        bool needsReimport{ false };
        bool needsStyleRefresh{ false };

        for (const auto colIdx : selectedCols)
            {
            if (colIdx < 0 || std::cmp_greater_equal(colIdx, m_columnInfo.size()))
                {
                continue;
                }

            auto& colInfo = m_columnInfo[static_cast<size_t>(colIdx)];

            if (sel == 4) // "Do not import"
                {
                if (!colInfo.m_excluded)
                    {
                    colInfo.m_excluded = true;
                    needsStyleRefresh = true;
                    }
                continue;
                }

            const bool wasExcluded = colInfo.m_excluded;
            colInfo.m_excluded = false;

            Data::Dataset::ColumnImportType newType = colInfo.m_type;
            switch (sel)
                {
            case 0:
                newType = Data::Dataset::ColumnImportType::String;
                break;
            case 1:
                newType = Data::Dataset::ColumnImportType::Discrete;
                break;
            case 2:
                newType = Data::Dataset::ColumnImportType::Numeric;
                break;
            case 3:
                newType = Data::Dataset::ColumnImportType::Date;
                break;
                }

            if (newType != colInfo.m_type)
                {
                colInfo.m_type = newType;
                colInfo.m_userOverridden = true;
                needsReimport = true;
                }
            else if (wasExcluded)
                {
                needsStyleRefresh = true;
                }
            }

        if (needsReimport)
            {
            RefreshPreviewFromColumnInfo();
            }
        else if (needsStyleRefresh)
            {
            ApplyExcludedColumnStyling();
            m_previewGrid->ForceRefresh();
            }
        }

    //----------------------------------------------
    void DatasetImportDlg::ApplyExcludedColumnStyling()
        {
        const wxColour excludedBg{ 220, 220, 220 };
        const wxColour excludedFg{ 160, 160, 160 };
        const int numRows = m_previewGrid->GetNumberRows();
        for (size_t col = 0; col < m_columnInfo.size(); ++col)
            {
            if (m_columnInfo[col].m_excluded)
                {
                for (int row = 0; row < numRows; ++row)
                    {
                    m_previewGrid->SetCellBackgroundColour(row, static_cast<int>(col), excludedBg);
                    m_previewGrid->SetCellTextColour(row, static_cast<int>(col), excludedFg);
                    }
                }
            else
                {
                for (int row = 0; row < numRows; ++row)
                    {
                    m_previewGrid->SetCellBackgroundColour(
                        row, static_cast<int>(col),
                        m_previewGrid->GetDefaultCellBackgroundColour());
                    m_previewGrid->SetCellTextColour(row, static_cast<int>(col),
                                                     m_previewGrid->GetDefaultCellTextColour());
                    }
                }
            }
        }

    //----------------------------------------------
    Data::Dataset::ColumnPreviewInfo DatasetImportDlg::GetColumnPreviewInfo() const
        {
        // return only non-excluded columns
        Data::Dataset::ColumnPreviewInfo result;
        result.reserve(m_columnInfo.size());
        std::ranges::copy_if(m_columnInfo, std::back_inserter(result),
                             [](const auto& col) { return !col.m_excluded; });
        return result;
        }

    //----------------------------------------------
    Data::ImportInfo DatasetImportDlg::GetImportInfo()
        {
        TransferDataFromWindow();
        const auto columnInfo = GetColumnPreviewInfo();

        auto importInfo = Data::Dataset::ImportInfoFromPreview(columnInfo);
        importInfo.SkipRows(static_cast<size_t>(m_skipRows));
        importInfo.TreatLeadingZerosAsText(m_leadingZeros);
        importInfo.TreatYearsAsText(m_yearsAsText);
        importInfo.MaxDiscreteValue(static_cast<uint16_t>(m_maxDiscrete));
        if (m_mdValues.empty())
            {
            importInfo.MDCodes(std::nullopt);
            }
        else
            {
            std::vector<std::wstring> mdCodes;
            wxStringTokenizer mdTokenizer(m_mdValues, L" ,;", wxTOKEN_STRTOK);
            while (mdTokenizer.HasMoreTokens())
                {
                mdCodes.push_back(mdTokenizer.GetNextToken().ToStdWstring());
                }
            importInfo.MDCodes(mdCodes);
            }
        if (m_idColumnChoice->GetSelection() > 0)
            {
            importInfo.IdColumn(m_idColumnChoice->GetStringSelection());
            }

        return importInfo;
        }

    //----------------------------------------------
    std::variant<wxString, size_t> DatasetImportDlg::GetWorksheet()
        {
        TransferDataFromWindow();
        if (!m_worksheetNames.empty() && m_worksheet != wxNOT_FOUND)
            {
            // 1-based index
            return static_cast<size_t>(m_worksheet + 1);
            }
        return static_cast<size_t>(1);
        }
    } // namespace Wisteria::UI
