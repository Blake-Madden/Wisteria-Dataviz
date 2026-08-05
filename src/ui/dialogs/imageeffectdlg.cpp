///////////////////////////////////////////////////////////////////////////////
// Name:        imageeffectdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imageeffectdlg.h"
#include "wx/valgen.h"
#include <wx/datetime.h>
#include <wx/wupdlock.h>

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
                                          nullptr, 0, wxGenericValidator{ &m_imageEffect });
        effectChoice->Append(_(L"None"));
        effectChoice->Append(_(L"Grayscale"));
        effectChoice->Append(_(L"Blur horizontal"));
        effectChoice->Append(_(L"Blur vertical"));
        effectChoice->Append(_(L"Sepia"));
        effectChoice->Append(_(L"Frosted glass"));
        effectChoice->Append(_(L"Oil painting"));
        effectChoice->Append(_(L"Color balance"));
        effectChoice->Append(_(L"Despeckle"));
        effectChoice->Append(_(L"Sharpen"));
        effectChoice->SetSelection(m_imageEffect);
        optionsSizer->Add(effectChoice, wxSizerFlags{}.CenterVertical());
        mainSizer->Add(optionsSizer, wxSizerFlags{}.Border());

        auto* cropBorderSizer = new wxStaticBoxSizer(wxVERTICAL, this);
        auto* cropCheckbox = new wxCheckBox(
            cropBorderSizer->GetStaticBox(), wxID_ANY, _(L"Crop border around image"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_cropImageBorder });
        cropBorderSizer->Add(cropCheckbox, wxSizerFlags{}.Border());

        auto* cropColorSizer = new wxBoxSizer(wxHORIZONTAL);
        m_cropBorderColorLabel =
            new wxStaticText(cropBorderSizer->GetStaticBox(), wxID_ANY, _(L"Border color:"));
        cropColorSizer->Add(m_cropBorderColorLabel,
                            wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_cropBorderColorCtrl =
            new wxColourPickerCtrl(cropBorderSizer->GetStaticBox(), wxID_ANY, m_cropBorderColor);
        cropColorSizer->Add(m_cropBorderColorCtrl, wxSizerFlags{}.CenterVertical());
        cropBorderSizer->Add(cropColorSizer, wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        auto* cropToleranceSizer = new wxBoxSizer(wxHORIZONTAL);
        m_cropBorderToleranceLabel = new wxStaticText(cropBorderSizer->GetStaticBox(), wxID_ANY,
                                                      _(L"Border color threshold:"));
        cropToleranceSizer->Add(m_cropBorderToleranceLabel,
                                wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_cropBorderToleranceCtrl =
            new wxSpinCtrl(cropBorderSizer->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 127, m_cropBorderTolerance);
        m_cropBorderToleranceCtrl->SetValidator(wxGenericValidator{ &m_cropBorderTolerance });
        cropToleranceSizer->Add(m_cropBorderToleranceCtrl, wxSizerFlags{}.CenterVertical());
        cropBorderSizer->Add(cropToleranceSizer,
                             wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(cropBorderSizer, wxSizerFlags{}.Expand().Border());

        const wxSize previewSize{ FromDIP(wxSize{ 512, 512 }) };

        auto* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_thumbnail = new Thumbnail(previewSizer->GetStaticBox(), m_originalImage,
                                    Wisteria::ClickMode::BrowseForImageFile, true, wxID_ANY,
                                    wxDefaultPosition, previewSize);
        previewSizer->Add(m_thumbnail, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(previewSizer, wxSizerFlags{ 1 }.Expand().Border());

        auto* imagePathSizer = new wxBoxSizer(wxHORIZONTAL);
        imagePathSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Image path:")),
                            wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_imagePathLabel = new wxStaticText(this, wxID_ANY, m_baseImagePath.GetFullPath());
        m_imagePathLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        imagePathSizer->Add(m_imagePathLabel, wxSizerFlags{ 1 }.CenterVertical());
        auto* imagePathBrowseButton = new wxButton(this, wxID_ANY, _(L"Browse..."));
        imagePathSizer->Add(imagePathBrowseButton, wxSizerFlags{}.CenterVertical().Border(wxLEFT));
        mainSizer->Add(imagePathSizer, wxSizerFlags{}.Expand().Border());

        auto* outputPathSizer = new wxBoxSizer(wxHORIZONTAL);
        outputPathSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Output image:")),
                             wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_outputPathPicker = new wxFilePickerCtrl(
            this, wxID_ANY, CreateDefaultOutputPath(m_baseImagePath), _(L"Select Output Image"),
            GraphItems::Image::GetImageFileFilter(), wxDefaultPosition, wxDefaultSize,
            wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT | wxFLP_USE_TEXTCTRL);
        outputPathSizer->Add(m_outputPathPicker, wxSizerFlags{ 1 }.CenterVertical());
        mainSizer->Add(outputPathSizer, wxSizerFlags{}.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        UpdateCropBorderControls();

        Bind(wxEVT_CHOICE,
             [this]([[maybe_unused]] const wxCommandEvent&)
             {
                 TransferDataFromWindow();
                 UpdatePreview();
             });

        Bind(wxEVT_CHECKBOX,
             [this]([[maybe_unused]] const wxCommandEvent&)
             {
                 TransferDataFromWindow();
                 UpdateCropBorderControls();
                 UpdatePreview();
             });

        m_cropBorderToleranceCtrl->Bind(wxEVT_SPINCTRL,
                                        [this]([[maybe_unused]] wxSpinEvent&)
                                        {
                                            TransferDataFromWindow();
                                            UpdatePreview();
                                        });

        m_cropBorderColorCtrl->Bind(wxEVT_COLOURPICKER_CHANGED,
                                    [this](const wxColourPickerEvent& event)
                                    {
                                        m_cropBorderColor = event.GetColour();
                                        UpdatePreview();
                                    });

        imagePathBrowseButton->Bind(wxEVT_BUTTON,
                                    [this]([[maybe_unused]] const wxCommandEvent&)
                                    {
                                        wxFileDialog fd(
                                            this, _(L"Select an Image"), m_baseImagePath.GetPath(),
                                            wxString{}, GraphItems::Image::GetImageFileFilter(),
                                            wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);
                                        if (fd.ShowModal() == wxID_OK)
                                            {
                                            SetSourceImage(fd.GetPath());
                                            }
                                    });

        m_thumbnail->Bind(wxEVT_THUMBNAIL_CHANGED,
                          [this]([[maybe_unused]] wxCommandEvent&)
                          {
                              if (!m_thumbnail->GetFilePath().empty() &&
                                  m_thumbnail->GetFilePath() != m_baseImagePath.GetFullPath())
                                  {
                                  SetSourceImage(m_thumbnail->GetFilePath());
                                  }
                          });

        Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] const wxCommandEvent&)
            {
                m_effectFilePath = m_outputPathPicker->GetPath();
                if (m_effectFilePath.empty())
                    {
                    wxMessageBox(_(L"Please select a path to save the image to."), _(L"Save"),
                                 wxOK | wxICON_EXCLAMATION);
                    return;
                    }
                if (wxFileExists(m_effectFilePath) &&
                    wxMessageBox(
                        wxString::Format(_(L"%s already exists.\nDo you want to replace it?"),
                                         m_effectFilePath),
                        _(L"Save"), wxYES_NO | wxICON_QUESTION) != wxYES)
                    {
                    return;
                    }

                const auto effectImg = GraphItems::Image::ApplyEffect(
                    static_cast<Wisteria::ImageEffect>(m_imageEffect), GetSourceImage());
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
        const wxWindowUpdateLocker noUpdates{ this };

        const auto effectImg = GraphItems::Image::ApplyEffect(
            static_cast<Wisteria::ImageEffect>(m_imageEffect), GetSourceImage());
        m_thumbnail->SetBitmap(effectImg);
        Layout();
        }

    //----------------------------------------
    wxImage ImageEffectDlg::GetSourceImage() const
        {
        return m_cropImageBorder ?
                   GraphItems::Image::CropImageBorder(m_originalImage,
                                                      static_cast<uint8_t>(m_cropBorderTolerance),
                                                      m_cropBorderColor) :
                   m_originalImage;
        }

    //----------------------------------------
    void ImageEffectDlg::UpdateCropBorderControls()
        {
        m_cropBorderToleranceLabel->Enable(m_cropImageBorder);
        m_cropBorderToleranceLabel->Refresh();
        m_cropBorderToleranceCtrl->Enable(m_cropImageBorder);
        m_cropBorderColorLabel->Enable(m_cropImageBorder);
        m_cropBorderColorLabel->Refresh();
        m_cropBorderColorCtrl->Enable(m_cropImageBorder);
        }

    //----------------------------------------
    void ImageEffectDlg::SetSourceImage(const wxString& path)
        {
        if (path.empty() || !wxFileExists(path))
            {
            return;
            }

        const wxWindowUpdateLocker noUpdates{ this };

        m_baseImagePath = wxFileName{ path };
        m_originalImage = GraphItems::Image::LoadFile(path);
        m_imagePathLabel->SetLabel(m_baseImagePath.GetFullPath());
        m_outputPathPicker->SetPath(CreateDefaultOutputPath(m_baseImagePath));
        UpdatePreview();
        }

    //----------------------------------------
    wxString ImageEffectDlg::CreateDefaultOutputPath(const wxFileName& sourcePath)
        {
        wxFileName outputPath{ sourcePath };
        outputPath.SetName(wxString::Format(L"%s_edited_%s", sourcePath.GetName(),
                                            wxDateTime::Now().Format(L"%Y%m%d%H%M%S")));
        return outputPath.GetFullPath();
        }
    } // namespace Wisteria::UI
