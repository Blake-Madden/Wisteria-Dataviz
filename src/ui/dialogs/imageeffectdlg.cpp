///////////////////////////////////////////////////////////////////////////////
// Name:        imageeffectdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imageeffectdlg.h"
#include "wx/valgen.h"

namespace Wisteria::UI
    {
    //----------------------------------------
    void ImageEffectDlg::CreateControls(const wxString& imgPath)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_baseImagePath = wxFileName{ imgPath };
        m_originalImage = Wisteria::GraphItems::Image::LoadFile(imgPath);

        auto* optionsSizer = new wxBoxSizer(wxHORIZONTAL);
        optionsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Effect:")),
                          wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        auto* effectChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                          nullptr, 0, wxGenericValidator(&m_imageEffect));
        effectChoice->Append(_(L"None"));
        effectChoice->Append(_(L"Grayscale"));
        effectChoice->Append(_(L"Blur horizontal"));
        effectChoice->Append(_(L"Blur vertical"));
        effectChoice->Append(_(L"Sepia"));
        effectChoice->Append(_(L"Frosted glass"));
        effectChoice->Append(_(L"Oil painting"));
        effectChoice->Append(_(L"Color balance"));
        effectChoice->SetSelection(m_imageEffect);
        optionsSizer->Add(effectChoice, wxSizerFlags{}.CenterVertical());
        mainSizer->Add(optionsSizer, wxSizerFlags{}.Border());

        auto* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_thumbnail = new Thumbnail(previewSizer->GetStaticBox(), m_originalImage,
                                    Wisteria::ClickMode::FullSizeViewable, false);
        previewSizer->Add(m_thumbnail, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(previewSizer, wxSizerFlags{ 1 }.Expand().Border());

        mainSizer->Add(new wxStaticText(this, wxID_STATIC,
                                        _(L"Select an effect to preview it.\n"
                                          "Click OK to apply the effect and save a new image.")),
                       wxSizerFlags{}.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        Bind(wxEVT_CHOICE,
             [this]([[maybe_unused]] const wxCommandEvent&)
             {
                 TransferDataFromWindow();
                 UpdatePreview();
             });

        Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] const wxCommandEvent&)
            {
                wxFileDialog fd(this, _(L"Select Output Image"), m_baseImagePath.GetPath(),
                                m_baseImagePath.GetName() + L"_edited." + m_baseImagePath.GetExt(),
                                GraphItems::Image::GetImageFileFilter(),
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_PREVIEW);
                if (fd.ShowModal() != wxID_OK)
                    {
                    return;
                    }

                m_effectFilePath = fd.GetPath();

                const auto effectImg = GraphItems::Image::ApplyEffect(
                    static_cast<Wisteria::ImageEffect>(m_imageEffect), m_originalImage);
                if (!effectImg.SaveFile(m_effectFilePath))
                    {
                    wxMessageBox(_(L"Unable to save image."), _(L"Save"), wxOK);
                    return;
                    }

                if (IsModal())
                    {
                    EndModal(wxID_OK);
                    }
                else
                    {
                    Show(false);
                    }
            },
            wxID_OK);
        }

    //----------------------------------------
    void ImageEffectDlg::UpdatePreview()
        {
        const auto effectImg = GraphItems::Image::ApplyEffect(
            static_cast<Wisteria::ImageEffect>(m_imageEffect), m_originalImage);
        m_thumbnail->SetBitmap(effectImg);
        }
    } // namespace Wisteria::UI
