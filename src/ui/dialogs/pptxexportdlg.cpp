///////////////////////////////////////////////////////////////////////////////
// Name:        pptxexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pptxexportdlg.h"
#include <utility>
#include <wx/valgen.h>
#include <wx/wupdlock.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    PptxExportDlg::PptxExportDlg(wxWindow* parent, PowerPointExportOptions options,
                                 const wxString& caption)
        : DialogWithHelp(parent, wxID_ANY, caption), m_options(std::move(options))
        {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_BLOCK_EVENTS);

        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //------------------------------------------------------
    void PptxExportDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        // slide size
        auto* slideSizeBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Slide Size"));

        wxArrayString slideSizeChoices;
        slideSizeChoices.Add(_(L"Widescreen (16:9)"));
        slideSizeChoices.Add(_(L"Standard (4:3)"));
        slideSizeChoices.Add(_(L"Custom"));
        m_slideSizeRadio =
            new wxRadioBox(slideSizeBox->GetStaticBox(), wxID_ANY, _(L"Preset"), wxDefaultPosition,
                           wxDefaultSize, slideSizeChoices, 1, wxRA_SPECIFY_COLS);
        m_slideSizeRadio->SetSelection(
            m_options.m_slideSize == PowerPointExportOptions::SlideSize::Widescreen16x9 ? 0 :
            m_options.m_slideSize == PowerPointExportOptions::SlideSize::Standard4x3    ? 1 :
                                                                                          2);
        slideSizeBox->Add(m_slideSizeRadio, wxSizerFlags{}.Expand().Border());

        auto* customSizeGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        m_customWidthLabel =
            new wxStaticText(slideSizeBox->GetStaticBox(), wxID_STATIC, _(L"Width (inches):"));
        customSizeGrid->Add(m_customWidthLabel, wxSizerFlags{}.CenterVertical());
        m_customWidthCtrl = new wxSpinCtrlDouble(slideSizeBox->GetStaticBox(), wxID_ANY, wxString{},
                                                 wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
                                                 1.0, PowerPointExportOptions::MAX_SLIDE_INCHES,
                                                 m_options.m_customWidthInches, 0.1);
        customSizeGrid->Add(m_customWidthCtrl, wxSizerFlags{}.Expand());

        m_customHeightLabel =
            new wxStaticText(slideSizeBox->GetStaticBox(), wxID_STATIC, _(L"Height (inches):"));
        customSizeGrid->Add(m_customHeightLabel, wxSizerFlags{}.CenterVertical());
        m_customHeightCtrl = new wxSpinCtrlDouble(
            slideSizeBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
            wxSP_ARROW_KEYS, 1.0, PowerPointExportOptions::MAX_SLIDE_INCHES,
            m_options.m_customHeightInches, 0.1);
        customSizeGrid->Add(m_customHeightCtrl, wxSizerFlags{}.Expand());

        slideSizeBox->Add(customSizeGrid, wxSizerFlags{}.Expand().Border());

        mainSizer->Add(slideSizeBox, wxSizerFlags{}.Expand().Border());

        m_slideSizeRadio->Bind(wxEVT_RADIOBOX,
                               [this](wxCommandEvent&) { UpdateSlideSizeControls(); });

        // document information
        auto* docInfoBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Document Information"));
        auto* docInfoGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        docInfoGrid->AddGrowableCol(1, 1);

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Title:")),
                         wxSizerFlags{}.CenterVertical());
        auto* titleCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           FromDIP(wxSize{ 350, -1 }), 0, wxGenericValidator{ &m_options.m_title });
        docInfoGrid->Add(titleCtrl, wxSizerFlags{}.Expand());

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Author:")),
                         wxSizerFlags{}.CenterVertical());
        auto* authorCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_options.m_author });
        docInfoGrid->Add(authorCtrl, wxSizerFlags{}.Expand());

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Subject:")),
                         wxSizerFlags{}.CenterVertical());
        auto* subjectCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_options.m_subject });
        docInfoGrid->Add(subjectCtrl, wxSizerFlags{}.Expand());

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Keywords:")),
                         wxSizerFlags{}.CenterVertical());
        auto* keywordsCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_options.m_keywords });
        docInfoGrid->Add(keywordsCtrl, wxSizerFlags{}.Expand());

        docInfoBox->Add(docInfoGrid, wxSizerFlags{}.Expand().Border());

        mainSizer->Add(docInfoBox, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

        // transitions
        auto* transitionsBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Transitions"));
        auto* transitionsGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        transitionsGrid->Add(
            new wxStaticText(transitionsBox->GetStaticBox(), wxID_STATIC, _(L"Effect:")),
            wxSizerFlags{}.CenterVertical());
        wxArrayString transitionChoices;
        transitionChoices.Add(_(L"None"));
        transitionChoices.Add(_(L"Fade"));
        transitionChoices.Add(_(L"Push"));
        transitionChoices.Add(_(L"Wipe"));
        transitionChoices.Add(_(L"Split"));
        transitionChoices.Add(_(L"Cut"));
        transitionChoices.Add(_(L"Morph"));
        m_transitionChoice = new wxChoice(transitionsBox->GetStaticBox(), wxID_ANY,
                                          wxDefaultPosition, wxDefaultSize, transitionChoices);
        m_transitionChoice->SetSelection(static_cast<int>(m_options.m_transition));
        transitionsGrid->Add(m_transitionChoice, wxSizerFlags{}.Expand());

        m_transitionSpeedLabel =
            new wxStaticText(transitionsBox->GetStaticBox(), wxID_STATIC, _(L"Speed:"));
        transitionsGrid->Add(m_transitionSpeedLabel, wxSizerFlags{}.CenterVertical());
        wxArrayString speedChoices;
        speedChoices.Add(_(L"Slow"));
        speedChoices.Add(_(L"Medium"));
        speedChoices.Add(_(L"Fast"));
        m_transitionSpeedChoice = new wxChoice(transitionsBox->GetStaticBox(), wxID_ANY,
                                               wxDefaultPosition, wxDefaultSize, speedChoices);
        m_transitionSpeedChoice->SetSelection(static_cast<int>(m_options.m_transitionSpeed));
        transitionsGrid->Add(m_transitionSpeedChoice, wxSizerFlags{}.Expand());

        transitionsBox->Add(transitionsGrid, wxSizerFlags{}.Expand().Border());

        m_transitionChoice->Bind(wxEVT_CHOICE,
                                 [this](wxCommandEvent&) { UpdateTransitionControls(); });

        m_advanceOnClickCheck = new wxCheckBox(
            transitionsBox->GetStaticBox(), wxID_ANY, _(L"Advance on mouse click"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_options.m_advanceOnClick });
        transitionsBox->Add(m_advanceOnClickCheck, wxSizerFlags{}.Border());

        auto* advanceAutoSizer = new wxBoxSizer(wxHORIZONTAL);
        m_advanceAutomaticallyCheck =
            new wxCheckBox(transitionsBox->GetStaticBox(), wxID_ANY,
                           _(L"Advance automatically after"), wxDefaultPosition, wxDefaultSize, 0,
                           wxGenericValidator{ &m_options.m_advanceAutomatically });
        advanceAutoSizer->Add(m_advanceAutomaticallyCheck, wxSizerFlags{}.CenterVertical());
        m_advanceSecondsCtrl = new wxSpinCtrl(
            transitionsBox->GetStaticBox(), wxID_ANY, std::to_wstring(m_options.m_advanceSeconds),
            wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 3600, m_options.m_advanceSeconds);
        m_advanceSecondsCtrl->SetValidator(wxGenericValidator{ &m_options.m_advanceSeconds });
        advanceAutoSizer->Add(m_advanceSecondsCtrl, wxSizerFlags{}.CenterVertical().Border(wxLEFT));
        advanceAutoSizer->Add(
            new wxStaticText(transitionsBox->GetStaticBox(), wxID_ANY, _(L"seconds")),
            wxSizerFlags{}.CenterVertical().Border(wxLEFT));
        transitionsBox->Add(advanceAutoSizer, wxSizerFlags{}.Border());

        m_advanceAutomaticallyCheck->Bind(wxEVT_CHECKBOX,
                                          [this](wxCommandEvent&) { UpdateTransitionControls(); });

        m_loopCheck =
            new wxCheckBox(transitionsBox->GetStaticBox(), wxID_ANY,
                           _(L"Loop continuously until Esc"), wxDefaultPosition, wxDefaultSize, 0,
                           wxGenericValidator{ &m_options.m_loopContinuously });
        transitionsBox->Add(m_loopCheck, wxSizerFlags{}.Border());

        mainSizer->Add(transitionsBox, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

        // notes
        auto* notesBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Notes"));
        auto* notesCheck = new wxCheckBox(
            notesBox->GetStaticBox(), wxID_ANY, _(L"Add chart descriptions as speaker notes"),
            wxDefaultPosition, wxDefaultSize, 0,
            wxGenericValidator{ &m_options.m_includeAccessibilityNotes });
        notesBox->Add(notesCheck, wxSizerFlags{}.Border());

        mainSizer->Add(notesBox, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        UpdateSlideSizeControls();
        UpdateTransitionControls();
        }

    //------------------------------------------------------
    void PptxExportDlg::UpdateSlideSizeControls()
        {
        const wxWindowUpdateLocker noUpdates{ this };

        const bool isCustom{ m_slideSizeRadio->GetSelection() == 2 };
        m_customWidthLabel->Enable(isCustom);
        m_customWidthCtrl->Enable(isCustom);
        m_customHeightLabel->Enable(isCustom);
        m_customHeightCtrl->Enable(isCustom);
        }

    //------------------------------------------------------
    void PptxExportDlg::UpdateTransitionControls()
        {
        const wxWindowUpdateLocker noUpdates{ this };

        const bool hasTransition{ m_transitionChoice->GetSelection() > 0 };
        m_transitionSpeedLabel->Enable(hasTransition);
        m_transitionSpeedChoice->Enable(hasTransition);

        m_advanceSecondsCtrl->Enable(m_advanceAutomaticallyCheck->GetValue());
        }

    //------------------------------------------------------
    bool PptxExportDlg::Validate()
        {
        m_options.m_slideSize = m_slideSizeRadio->GetSelection() == 0 ?
                                    PowerPointExportOptions::SlideSize::Widescreen16x9 :
                                m_slideSizeRadio->GetSelection() == 1 ?
                                    PowerPointExportOptions::SlideSize::Standard4x3 :
                                    PowerPointExportOptions::SlideSize::Custom;
        m_options.m_customWidthInches = m_customWidthCtrl->GetValue();
        m_options.m_customHeightInches = m_customHeightCtrl->GetValue();

        m_options.m_transition =
            static_cast<PowerPointExportOptions::Transition>(m_transitionChoice->GetSelection());
        m_options.m_transitionSpeed = static_cast<PowerPointExportOptions::TransitionSpeed>(
            m_transitionSpeedChoice->GetSelection());

        return wxDialog::Validate();
        }
    } // namespace Wisteria::UI
