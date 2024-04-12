///////////////////////////////////////////////////////////////////////////////
// Name:        edit_text_dlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "excelpreviewdlg.h"
#include <wx/spinctrl.h>
#include <wx/busyinfo.h>

//-------------------------------------------------------------
void ExcelPreviewDlg::OnChangeImportMethod([[maybe_unused]] wxCommandEvent& event)
    {
    Validate();
    TransferDataFromWindow();
    m_grid->Enable(m_importMethod == 1);
    m_grid->SetFocus();
    }

//-------------------------------------------------------------
void ExcelPreviewDlg::OnOK([[maybe_unused]] wxCommandEvent& event)
    {
    Validate();
    TransferDataFromWindow();

    // individual cells
    wxGridCellCoordsArray selCells = m_grid->GetSelectedCells();
    for (size_t i = 0; i < selCells.Count(); ++i)
        { m_selectedCells.push_back(selCells[i]); }
    // rows
    wxArrayInt selRows = m_grid->GetSelectedRows();
    for (size_t i = 0; i < selRows.Count(); ++i)
        { m_selectedRows.insert(selRows[i]); }
    // columns
    wxArrayInt selCols = m_grid->GetSelectedCols();
    for (size_t i = 0; i < selCols.Count(); ++i)
        { m_selectedColumns.insert(selCols[i]); }
    // selected blocks
    wxGridCellCoordsArray topLeftCorners = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray bottomRightCorners = m_grid->GetSelectionBlockBottomRight();
    m_selectedBlocks.resize(topLeftCorners.Count());
    for (size_t i = 0; i < topLeftCorners.Count(); ++i)
        { m_selectedBlocks[i] = std::make_pair(topLeftCorners[i], bottomRightCorners[i]); }

    if (IsImportingOnlySelectedCells() && m_selectedCells.empty() &&
        m_selectedRows.empty() && m_selectedColumns.empty() && m_selectedBlocks.empty())
        {
        wxMessageBox(_(L"Please highlight cells to import."),
            _(L"Error"), wxOK|wxICON_EXCLAMATION);
        return;
        }

    EndModal(wxID_OK);
    }

//-------------------------------------------------------------
void ExcelPreviewDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxArrayString choices;
    choices.Add(_(L"All text cells"));
    choices.Add(_(L"Only highlighted cells"));
    mainSizer->Add(new wxRadioBox(this, wxID_ANY, _(L"Import:"), wxDefaultPosition,
        wxDefaultSize, choices, 0, wxRA_SPECIFY_ROWS, wxGenericValidator(&m_importMethod)), 0,
        wxLEFT|wxTOP|wxBOTTOM, wxSizerFlags::GetDefaultBorder());
    if (m_wrk->size() > 0)
        {
        m_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(800,400)));
        m_grid->SetTable(new ExcelTable(m_wrk, m_excelFile), true);
        m_grid->EnableEditing(false);
        m_grid->SetDefaultCellOverflow(false);
        m_grid->Enable(m_importMethod == 1);

        mainSizer->Add(m_grid, 1, wxEXPAND);
        }

    mainSizer->Add(new wxStaticText(this, wxID_STATIC,
        _(L"Note: only text cells are being shown and are truncated here for display purposes.")), 0,
        wxALL, wxSizerFlags::GetDefaultBorder());
    mainSizer->Add(CreateButtonSizer(wxOK|wxHELP), 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    mainSizer->SetMinSize(GetSize());
    SetSizer(mainSizer);
    }
