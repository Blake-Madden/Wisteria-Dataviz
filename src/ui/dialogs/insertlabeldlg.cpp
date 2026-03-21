///////////////////////////////////////////////////////////////////////////////
// Name:        insertlabeldlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertlabeldlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertLabelDlg::InsertLabelDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode, const bool includePageOptions)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode),
          m_includePageOptions(includePageOptions)
        {
        CreateControls();
        FinalizeControls();
        TransferDataToWindow();

        const auto currentSize = GetSize();
        SetSize(currentSize.GetWidth() * 2, currentSize.GetHeight());
        SetMinSize(wxSize{ currentSize.GetWidth() * 2, currentSize.GetHeight() });

        Centre();
        }

    //-------------------------------------------
    void InsertLabelDlg::CreateControls()
        {
        InsertItemDlg::CreateControls();

        auto* labelPage = new wxPanel(GetSideBarBook());
        auto* labelSizer = new wxBoxSizer(wxVERTICAL);
        labelPage->SetSizer(labelSizer);
        GetSideBarBook()->AddPage(labelPage, _(L"Label Options"), ID_LABEL_SECTION, true);

        if (!m_includePageOptions)
            {
            GetSideBarBook()->DeletePage(0);
            }

        // text
        labelSizer->Add(new wxStaticText(labelPage, wxID_ANY, _(L"Text:")),
                        wxSizerFlags{}.Border(wxLEFT | wxTOP));
        m_textCtrl = new wxTextCtrl(labelPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                    wxSize{ FromDIP(300), FromDIP(100) }, wxTE_MULTILINE);
        labelSizer->Add(m_textCtrl, wxSizerFlags{ 1 }.Expand().Border());

        // font options
        auto* fontGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        fontGrid->Add(new wxStaticText(labelPage, wxID_ANY, _(L"Font:")),
                      wxSizerFlags{}.CenterVertical());
        m_fontPicker = new wxFontPickerCtrl(labelPage, wxID_ANY,
                                            wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        fontGrid->Add(m_fontPicker);

        fontGrid->Add(new wxStaticText(labelPage, wxID_ANY, _(L"Font color:")),
                      wxSizerFlags{}.CenterVertical());
        m_fontColorPicker = new wxColourPickerCtrl(labelPage, wxID_ANY, *wxBLACK);
        fontGrid->Add(m_fontColorPicker);

        fontGrid->Add(new wxStaticText(labelPage, wxID_ANY, _(L"Background color:")),
                      wxSizerFlags{}.CenterVertical());
        m_bgColorPicker = new wxColourPickerCtrl(labelPage, wxID_ANY, wxTransparentColour);
        fontGrid->Add(m_bgColorPicker);

        fontGrid->Add(new wxStaticText(labelPage, wxID_ANY, _(L"Alignment:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* alignChoice = new wxChoice(labelPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             0, nullptr, 0, wxGenericValidator(&m_alignment));
            alignChoice->Append(_(L"Left"));
            alignChoice->Append(_(L"Right"));
            alignChoice->Append(_(L"Center"));
            fontGrid->Add(alignChoice);
            }

        labelSizer->Add(fontGrid, wxSizerFlags{}.Border());

        // header options
        auto* headerBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Header"));

        auto* enableHeaderCheck = new wxCheckBox(
            headerBox->GetStaticBox(), wxID_ANY, _(L"Treat first line as header"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&m_headerEnabled));
        headerBox->Add(enableHeaderCheck, wxSizerFlags{}.Border());

        auto* headerGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        headerGrid->Add(new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Font:")),
                        wxSizerFlags{}.CenterVertical());
        m_headerFontPicker = new wxFontPickerCtrl(
            headerBox->GetStaticBox(), wxID_ANY, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        headerGrid->Add(m_headerFontPicker);

        headerGrid->Add(new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                        wxSizerFlags{}.CenterVertical());
        m_headerColorPicker = new wxColourPickerCtrl(headerBox->GetStaticBox(), wxID_ANY, *wxBLACK);
        headerGrid->Add(m_headerColorPicker);

        headerGrid->Add(new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Alignment:")),
                        wxSizerFlags{}.CenterVertical());
        m_headerAlignmentChoice =
            new wxChoice(headerBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                         nullptr, 0, wxGenericValidator(&m_headerAlignment));
        m_headerAlignmentChoice->Append(_(L"Left"));
        m_headerAlignmentChoice->Append(_(L"Right"));
        m_headerAlignmentChoice->Append(_(L"Center"));
        headerGrid->Add(m_headerAlignmentChoice);

        headerGrid->Add(
            new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Relative scaling:")),
            wxSizerFlags{}.CenterVertical());
        m_headerScalingSpin = new wxSpinCtrlDouble(headerBox->GetStaticBox(), wxID_ANY,
                                                   wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                                   wxSP_ARROW_KEYS, 0.5, 3.0, 1.0, 0.1);
        headerGrid->Add(m_headerScalingSpin);

        headerBox->Add(headerGrid, wxSizerFlags{}.Border());
        labelSizer->Add(headerBox, wxSizerFlags{}.Expand().Border());

        // header controls start disabled
        OnEnableHeader(false);

        // bind events
        enableHeaderCheck->Bind(wxEVT_CHECKBOX,
                                [this](wxCommandEvent& evt) { OnEnableHeader(evt.IsChecked()); });
        }

    //-------------------------------------------
    void InsertLabelDlg::OnEnableHeader(const bool enable)
        {
        if (m_headerFontPicker != nullptr)
            {
            m_headerFontPicker->Enable(enable);
            }
        if (m_headerColorPicker != nullptr)
            {
            m_headerColorPicker->Enable(enable);
            }
        if (m_headerAlignmentChoice != nullptr)
            {
            m_headerAlignmentChoice->Enable(enable);
            }
        if (m_headerScalingSpin != nullptr)
            {
            m_headerScalingSpin->Enable(enable);
            }
        }

    //-------------------------------------------
    bool InsertLabelDlg::Validate()
        {
        if (m_textCtrl->GetValue().empty())
            {
            wxMessageBox(_(L"Please enter label text."), _(L"Text Required"), wxOK | wxICON_WARNING,
                         this);
            m_textCtrl->SetFocus();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertLabelDlg::LoadFromLabel(const Wisteria::GraphItems::Label& label, Canvas* canvas)
        {
        if (m_includePageOptions)
            {
            LoadPageOptions(label, canvas);
            }

        if (m_textCtrl != nullptr)
            {
            const auto textTemplate = label.GetPropertyTemplate(L"text");
            m_textCtrl->SetValue(textTemplate.empty() ? label.GetText() : textTemplate);
            }
        if (m_fontPicker != nullptr)
            {
            m_fontPicker->SetSelectedFont(label.GetFont());
            }
        if (m_fontColorPicker != nullptr)
            {
            m_fontColorPicker->SetColour(label.GetFontColor());
            }
        if (m_bgColorPicker != nullptr)
            {
            m_bgColorPicker->SetColour(label.GetFontBackgroundColor());
            }
        m_alignment = static_cast<int>(label.GetTextAlignment());

        // header
        const auto& headerInfo = label.GetHeaderInfo();
        m_headerEnabled = headerInfo.IsEnabled();
        if (m_headerFontPicker != nullptr && headerInfo.GetFont().IsOk())
            {
            m_headerFontPicker->SetSelectedFont(headerInfo.GetFont());
            }
        if (m_headerColorPicker != nullptr && headerInfo.GetFontColor().IsOk())
            {
            m_headerColorPicker->SetColour(headerInfo.GetFontColor());
            }
        m_headerAlignment = static_cast<int>(headerInfo.GetLabelAlignment());
        if (m_headerScalingSpin != nullptr)
            {
            m_headerScalingSpin->SetValue(headerInfo.GetRelativeScaling());
            }

        OnEnableHeader(m_headerEnabled);

        TransferDataToWindow();
        }

    //-------------------------------------------
    void InsertLabelDlg::ApplyToLabel(Wisteria::GraphItems::Label& label) const
        {
        label.SetText(GetLabelText());
        label.GetFont() = GetLabelFont();
        label.SetFontColor(GetFontColor());
        label.SetFontBackgroundColor(GetBackgroundColor());
        label.SetTextAlignment(GetLabelAlignment());

        auto& headerInfo = label.GetHeaderInfo();
        headerInfo.Enable(IsHeaderEnabled());
        if (IsHeaderEnabled())
            {
            headerInfo.Font(GetHeaderFont());
            headerInfo.FontColor(GetHeaderFontColor());
            headerInfo.LabelAlignment(GetHeaderAlignment());
            headerInfo.RelativeScaling(GetHeaderScaling());
            }
        }
    } // namespace Wisteria::UI
