/////////////////////////////////////////////////////////////////////////////
// Name:        gridexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "gridexportdlg.h"

using namespace Wisteria::UI;

GridExportDlg::GridExportDlg(wxWindow* parent, int rowCount, int columnCount,
                             const GridExportFormat& exportFormat, wxWindowID id /*= wxID_ANY*/,
                             const wxString& caption /*= _(L"List Export Options")*/,
                             const wxPoint& pos /*= wxDefaultPosition*/,
                             const wxSize& size /*= wxDefaultSize*/,
                             long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN*/)
    : m_exportFormat(exportFormat)
    {
    m_options.m_toRow = rowCount;
    m_options.m_toColumn = columnCount;
    SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS | wxWS_EX_CONTEXTHELP);
    DialogWithHelp::Create(parent, id, caption, pos, size, style);

    CreateControls();
    Centre();

    // setup the radio buttons' subcontrols' enablablement
    Bind(
        wxEVT_RADIOBUTTON,
        [this]([[maybe_unused]] wxCommandEvent&)
        {
            m_rangeBoxSizer->GetStaticBox()->Enable(false);
            for (auto& child : m_rangeBoxSizer->GetStaticBox()->GetChildren())
                {
                child->Enable(false);
                }
            m_paginateCheckBox->Enable();
        },
        ControlIDs::ID_EXPORT_ALL_OPTION);

    Bind(
        wxEVT_RADIOBUTTON,
        [this]([[maybe_unused]] wxCommandEvent&)
        {
            m_rangeBoxSizer->GetStaticBox()->Enable();
            for (auto& child : m_rangeBoxSizer->GetStaticBox()->GetChildren())
                {
                child->Enable();
                }
            auto rangeWindow =
                FindWindowById(ControlIDs::ID_ROWS_FROM_SPIN, m_rangeBoxSizer->GetStaticBox());
            if (rangeWindow)
                {
                rangeWindow->Enable(false);
                }
            rangeWindow =
                FindWindowById(ControlIDs::ID_ROWS_FROM_LABEL, m_rangeBoxSizer->GetStaticBox());
            if (rangeWindow)
                {
                rangeWindow->Enable(false);
                }
            rangeWindow =
                FindWindowById(ControlIDs::ID_ROWS_TO_SPIN, m_rangeBoxSizer->GetStaticBox());
            if (rangeWindow)
                {
                rangeWindow->Enable(false);
                }
            rangeWindow =
                FindWindowById(ControlIDs::ID_ROWS_TO_LABEL, m_rangeBoxSizer->GetStaticBox());
            if (rangeWindow)
                {
                rangeWindow->Enable(false);
                }
            m_paginateCheckBox->Enable(false);
        },
        ControlIDs::ID_EXPORT_SELECTED_OPTION);

    Bind(
        wxEVT_RADIOBUTTON,
        [this]([[maybe_unused]] wxCommandEvent&)
        {
            m_rangeBoxSizer->GetStaticBox()->Enable();
            for (auto& child : m_rangeBoxSizer->GetStaticBox()->GetChildren())
                {
                child->Enable();
                }
            m_paginateCheckBox->Enable();
        },
        ControlIDs::ID_EXPORT_RANGE_OPTION);
    }

// Creates the controls and sizers
//-------------------------------------------------------------
void GridExportDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* optionsSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(optionsSizer, wxSizerFlags(1).Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    wxCheckBox* columnHeaderCheck = new wxCheckBox(
        this, wxID_ANY, _(L"&Include column headers"), wxDefaultPosition, wxDefaultSize,
        wxCHK_2STATE, wxGenericValidator(&m_options.m_includeColumnHeaders));
    optionsSizer->Add(columnHeaderCheck,
                      wxSizerFlags().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    optionsSizer->Add(new wxRadioButton(this, ControlIDs::ID_EXPORT_ALL_OPTION,
                                        _(L"&Export all rows"), wxDefaultPosition, wxDefaultSize,
                                        wxRB_GROUP, wxGenericValidator(&m_options.m_exportAll)),
                      wxSizerFlags().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    optionsSizer->Add(new wxRadioButton(this, ControlIDs::ID_EXPORT_SELECTED_OPTION,
                                        _(L"Export &selected rows"), wxDefaultPosition,
                                        wxDefaultSize, 0,
                                        wxGenericValidator(&m_options.m_exportSelected)),
                      wxSizerFlags().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    optionsSizer->Add(new wxRadioButton(this, ControlIDs::ID_EXPORT_RANGE_OPTION,
                                        _(L"Export a &range of rows"), wxDefaultPosition,
                                        wxDefaultSize, 0,
                                        wxGenericValidator(&m_options.m_exportRange)),
                      wxSizerFlags().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    m_rangeBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Range:"));
    optionsSizer->Add(m_rangeBoxSizer,
                      wxSizerFlags(1).Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    wxFlexGridSizer* rangeOptionsSizer = new wxFlexGridSizer(2, 4, 5, 5);
    m_rangeBoxSizer->Add(rangeOptionsSizer,
                         wxSizerFlags(1).Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    // row options
    rangeOptionsSizer->Add(
        new wxStaticText(m_rangeBoxSizer->GetStaticBox(), ID_ROWS_FROM_LABEL, _(L"Rows: from")),
        wxSizerFlags().CenterVertical());

    auto fromRowSpinCtrl =
        new wxSpinCtrl(m_rangeBoxSizer->GetStaticBox(), ControlIDs::ID_ROWS_FROM_SPIN, L"1",
                       wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, m_options.m_toRow, 1);
    fromRowSpinCtrl->SetValidator(wxGenericValidator(&m_options.m_fromRow));
    rangeOptionsSizer->Add(fromRowSpinCtrl, wxSizerFlags().Expand());

    rangeOptionsSizer->Add(
        new wxStaticText(m_rangeBoxSizer->GetStaticBox(), ID_ROWS_TO_LABEL, _(L"to")),
        wxSizerFlags().CenterVertical());

    auto toRowSpinCtrl =
        new wxSpinCtrl(m_rangeBoxSizer->GetStaticBox(), ControlIDs::ID_ROWS_TO_SPIN, L"1",
                       wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, m_options.m_toRow, 1);
    toRowSpinCtrl->SetValidator(wxGenericValidator(&m_options.m_toRow));
    rangeOptionsSizer->Add(toRowSpinCtrl, wxSizerFlags().Expand());

    // column options
    rangeOptionsSizer->Add(
        new wxStaticText(m_rangeBoxSizer->GetStaticBox(), wxID_STATIC, _(L"Columns: from")),
        wxSizerFlags().CenterVertical());

    auto fromColumnSpinCtrl =
        new wxSpinCtrl(m_rangeBoxSizer->GetStaticBox(), wxID_ANY, L"1", wxDefaultPosition,
                       wxDefaultSize, wxSP_ARROW_KEYS, 1, m_options.m_toColumn, 1);
    fromColumnSpinCtrl->SetValidator(wxGenericValidator(&m_options.m_fromColumn));
    rangeOptionsSizer->Add(fromColumnSpinCtrl, wxSizerFlags().Expand());

    rangeOptionsSizer->Add(new wxStaticText(m_rangeBoxSizer->GetStaticBox(), wxID_STATIC, _(L"to")),
                           wxSizerFlags().CenterVertical());

    auto toColumnSpinCtrl =
        new wxSpinCtrl(m_rangeBoxSizer->GetStaticBox(), wxID_ANY, L"1", wxDefaultPosition,
                       wxDefaultSize, wxSP_ARROW_KEYS, 1, m_options.m_toColumn, 1);
    toColumnSpinCtrl->SetValidator(wxGenericValidator(&m_options.m_toColumn));
    rangeOptionsSizer->Add(toColumnSpinCtrl, wxSizerFlags().Expand());

    if (m_exportFormat == GridExportFormat::ExportHtml)
        {
        m_paginateCheckBox = new wxCheckBox(
            this, wxID_ANY, _(L"&Paginate using printer settings"), wxDefaultPosition,
            wxDefaultSize, wxCHK_2STATE, wxGenericValidator(&m_options.m_pageUsingPrinterSettings));
        optionsSizer->Add(m_paginateCheckBox,
                          wxSizerFlags().Border(wxALL, wxSizerFlags::GetDefaultBorder()));
        }

    m_rangeBoxSizer->GetStaticBox()->Enable(false);
    for (auto& child : m_rangeBoxSizer->GetStaticBox()->GetChildren())
        {
        child->Enable(false);
        }

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                   wxSizerFlags().Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    SetSizerAndFit(mainSizer);
    }
