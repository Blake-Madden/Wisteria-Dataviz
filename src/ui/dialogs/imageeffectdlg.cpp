///////////////////////////////////////////////////////////////////////////////
// Name:        imageeffectdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imageeffectdlg.h"
#include "imageexportdlg.h"
#include "wx/valgen.h"
#include <wx/artprov.h>
#include <wx/datetime.h>
#include <wx/quantize.h>
#include <wx/utils.h>
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
        m_pickColorButton = new wxButton(cropBorderSizer->GetStaticBox(), wxID_ANY);
        m_pickColorButton->SetBitmap(
            wxArtProvider::GetBitmapBundle(L"ID_COLOR_PICKER", wxART_BUTTON));
        m_pickColorButton->SetToolTip(_(L"Pick a color from the image"));
        cropColorSizer->Add(m_pickColorButton, wxSizerFlags{}.CenterVertical().Border(wxLEFT));
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

        auto* gutterShadowSizer = new wxStaticBoxSizer(wxVERTICAL, this);
        auto* gutterShadowCheckbox = new wxCheckBox(
            gutterShadowSizer->GetStaticBox(), wxID_ANY, _(L"Remove gutter shadow"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_removeGutterShadow });
        gutterShadowSizer->Add(gutterShadowCheckbox, wxSizerFlags{}.Border());

        auto* gutterSideSizer = new wxBoxSizer(wxHORIZONTAL);
        m_gutterSideLabel =
            new wxStaticText(gutterShadowSizer->GetStaticBox(), wxID_ANY, _(L"Gutter location:"));
        gutterSideSizer->Add(m_gutterSideLabel, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_gutterSideChoice =
            new wxChoice(gutterShadowSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                         wxDefaultSize, 0, nullptr, 0, wxGenericValidator{ &m_gutterSide });
        m_gutterSideChoice->Append(_(L"Left"));
        m_gutterSideChoice->Append(_(L"Right"));
        m_gutterSideChoice->Append(_(L"Center"));
        m_gutterSideChoice->SetSelection(m_gutterSide);
        gutterSideSizer->Add(m_gutterSideChoice, wxSizerFlags{}.CenterVertical());
        gutterShadowSizer->Add(gutterSideSizer, wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        auto* gutterWidthSizer = new wxBoxSizer(wxHORIZONTAL);
        m_gutterWidthLabel =
            new wxStaticText(gutterShadowSizer->GetStaticBox(), wxID_ANY, _(L"Gutter width (%):"));
        gutterWidthSizer->Add(m_gutterWidthLabel, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_gutterWidthCtrl =
            new wxSpinCtrl(gutterShadowSizer->GetStaticBox(), wxID_ANY, wxString{},
                           wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 50, m_gutterWidth);
        m_gutterWidthCtrl->SetValidator(wxGenericValidator{ &m_gutterWidth });
        gutterWidthSizer->Add(m_gutterWidthCtrl, wxSizerFlags{}.CenterVertical());
        gutterShadowSizer->Add(gutterWidthSizer,
                               wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        auto* cropAndGutterSizer = new wxBoxSizer(wxHORIZONTAL);
        cropAndGutterSizer->Add(cropBorderSizer, wxSizerFlags{ 1 }.Expand());
        cropAndGutterSizer->Add(gutterShadowSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT));
        mainSizer->Add(cropAndGutterSizer, wxSizerFlags{}.Expand().Border());

        auto* bleedThroughSizer = new wxStaticBoxSizer(wxVERTICAL, this);
        auto* bleedThroughCheckbox = new wxCheckBox(
            bleedThroughSizer->GetStaticBox(), wxID_ANY, _(L"Reduce bleed-through"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_reduceBleedThrough });
        bleedThroughSizer->Add(bleedThroughCheckbox, wxSizerFlags{}.Border());

        auto* bleedThroughWhitePointSizer = new wxBoxSizer(wxHORIZONTAL);
        m_bleedThroughWhitePointLabel =
            new wxStaticText(bleedThroughSizer->GetStaticBox(), wxID_ANY, _(L"Sensitivity:"));
        bleedThroughWhitePointSizer->Add(m_bleedThroughWhitePointLabel,
                                         wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_bleedThroughWhitePointCtrl = new wxSpinCtrl(
            bleedThroughSizer->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS, 1, 254, m_bleedThroughWhitePoint);
        m_bleedThroughWhitePointCtrl->SetValidator(wxGenericValidator{ &m_bleedThroughWhitePoint });
        bleedThroughWhitePointSizer->Add(m_bleedThroughWhitePointCtrl,
                                         wxSizerFlags{}.CenterVertical());
        bleedThroughSizer->Add(bleedThroughWhitePointSizer,
                               wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(bleedThroughSizer, wxSizerFlags{}.Expand().Border());

        auto* binarizeSizer = new wxStaticBoxSizer(wxVERTICAL, this);
        auto* binarizeCheckbox = new wxCheckBox(
            binarizeSizer->GetStaticBox(), wxID_ANY, _(L"Otsu binarize (black && white)"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_binarize });
        binarizeSizer->Add(binarizeCheckbox, wxSizerFlags{}.Border());

        auto* binarizeThresholdSizer = new wxBoxSizer(wxHORIZONTAL);
        m_binarizeThresholdLabel =
            new wxStaticText(binarizeSizer->GetStaticBox(), wxID_ANY, _(L"Threshold adjustment:"));
        binarizeThresholdSizer->Add(m_binarizeThresholdLabel,
                                    wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_binarizeThresholdCtrl = new wxSpinCtrl(
            binarizeSizer->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
            wxSP_ARROW_KEYS, -100, 100, m_binarizeThresholdAdjustment);
        m_binarizeThresholdCtrl->SetValidator(wxGenericValidator{ &m_binarizeThresholdAdjustment });
        binarizeThresholdSizer->Add(m_binarizeThresholdCtrl, wxSizerFlags{}.CenterVertical());
        binarizeSizer->Add(binarizeThresholdSizer,
                           wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(binarizeSizer, wxSizerFlags{}.Expand().Border());

        const wxSize previewSize{ FromDIP(wxSize{ 512, 512 }) };

        auto* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_thumbnail = new Thumbnail(previewSizer->GetStaticBox(), m_originalImage,
                                    Wisteria::ClickMode::FullSizeViewable, false, wxID_ANY,
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
        m_outputPathLabel = new wxStaticText(this, wxID_ANY, _(L"Output image:"));
        outputPathSizer->Add(m_outputPathLabel, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_outputPathPicker = new wxFilePickerCtrl(
            this, wxID_ANY, CreateDefaultOutputPath(m_baseImagePath), _(L"Select Output Image"),
            GraphItems::Image::GetImageFileFilter(), wxDefaultPosition, wxDefaultSize,
            wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT | wxFLP_USE_TEXTCTRL);
        outputPathSizer->Add(m_outputPathPicker, wxSizerFlags{ 1 }.CenterVertical());
        mainSizer->Add(outputPathSizer, wxSizerFlags{}.Expand().Border());

        mainSizer->Add(new wxCheckBox(this, wxID_ANY, _("Save image inplace"), wxDefaultPosition,
                                      wxDefaultSize, 0, wxGenericValidator{ &m_saveInplace }),
                       wxSizerFlags{}.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        UpdateCropBorderControls();
        UpdateGutterShadowControls();
        UpdateBleedThroughControls();
        UpdateBinarizeControls();
        UpdateSaveInplaceControls();

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
                 UpdateGutterShadowControls();
                 UpdateBleedThroughControls();
                 UpdateBinarizeControls();
                 UpdateSaveInplaceControls();
                 UpdatePreview();
             });

        m_cropBorderToleranceCtrl->Bind(wxEVT_SPINCTRL,
                                        [this]([[maybe_unused]] wxSpinEvent&)
                                        {
                                            TransferDataFromWindow();
                                            UpdatePreview();
                                        });

        m_cropBorderToleranceCtrl->Bind(wxEVT_TEXT,
                                        [this]([[maybe_unused]] wxCommandEvent&)
                                        {
                                            TransferDataFromWindow();
                                            UpdatePreview();
                                        });

        m_gutterWidthCtrl->Bind(wxEVT_SPINCTRL,
                                [this]([[maybe_unused]] wxSpinEvent&)
                                {
                                    TransferDataFromWindow();
                                    UpdatePreview();
                                });

        m_gutterWidthCtrl->Bind(wxEVT_TEXT,
                                [this]([[maybe_unused]] wxCommandEvent&)
                                {
                                    TransferDataFromWindow();
                                    UpdatePreview();
                                });

        m_bleedThroughWhitePointCtrl->Bind(wxEVT_SPINCTRL,
                                           [this]([[maybe_unused]] wxSpinEvent&)
                                           {
                                               TransferDataFromWindow();
                                               UpdatePreview();
                                           });

        m_bleedThroughWhitePointCtrl->Bind(wxEVT_TEXT,
                                           [this]([[maybe_unused]] wxCommandEvent&)
                                           {
                                               TransferDataFromWindow();
                                               UpdatePreview();
                                           });

        m_binarizeThresholdCtrl->Bind(wxEVT_SPINCTRL,
                                      [this]([[maybe_unused]] wxSpinEvent&)
                                      {
                                          TransferDataFromWindow();
                                          UpdatePreview();
                                      });

        m_binarizeThresholdCtrl->Bind(wxEVT_TEXT,
                                      [this]([[maybe_unused]] wxCommandEvent&)
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

        m_pickColorButton->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] const wxCommandEvent&)
                                { m_thumbnail->SetColorPickingMode(true); });

        m_thumbnail->Bind(wxEVT_THUMBNAIL_COLOR_PICKED,
                          [this](const wxColourPickerEvent& event)
                          {
                              m_cropBorderColor = event.GetColour();
                              m_cropBorderColorCtrl->SetColour(m_cropBorderColor);
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
                TransferDataFromWindow();
                m_effectFilePath =
                    m_saveInplace ? m_baseImagePath.GetFullPath() : m_outputPathPicker->GetPath();
                if (m_effectFilePath.empty())
                    {
                    wxMessageBox(_(L"Please select a path to save the image to."), _(L"Save"),
                                 wxOK | wxICON_EXCLAMATION);
                    return;
                    }
                if (!m_saveInplace && wxFileName::Exists(m_effectFilePath) &&
                    wxMessageBox(
                        wxString::Format(_(L"%s already exists.\nDo you want to replace it?"),
                                         m_effectFilePath),
                        _(L"Save"), wxYES_NO | wxICON_QUESTION) != wxYES)
                    {
                    return;
                    }

                auto effectImg = GraphItems::Image::ApplyEffect(
                    static_cast<Wisteria::ImageEffect>(m_imageEffect), GetSourceImage());

                effectImg.SetOption(wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES);
                effectImg.SetOption(wxIMAGE_OPTION_RESOLUTIONX,
                                    Settings::GetImageResolutionDPI().GetWidth());
                effectImg.SetOption(wxIMAGE_OPTION_RESOLUTIONY,
                                    Settings::GetImageResolutionDPI().GetHeight());

                wxString outputExt{ wxFileName{ m_effectFilePath }.GetExt() };
                const wxBitmapType outputImageType =
                    GraphItems::Image::GetImageFileTypeFromExtension(outputExt);
                if (outputImageType == wxBITMAP_TYPE_TIF)
                    {
                    effectImg.SetOption(wxIMAGE_OPTION_COMPRESSION,
                                        static_cast<int>(TiffCompression::CompressionNone));
                    }
                else if (outputImageType == wxBITMAP_TYPE_JPEG)
                    {
                    effectImg.SetOption(wxIMAGE_OPTION_QUALITY, 100);
                    }
                else if (outputImageType == wxBITMAP_TYPE_PNG)
                    {
                    effectImg.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
                    }
                else if (outputImageType == wxBITMAP_TYPE_GIF)
                    {
                    wxQuantize::Quantize(effectImg, effectImg, 256);
                    effectImg.ConvertAlphaToMask();
                    }

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
        wxImage img{ m_cropImageBorder ?
                         GraphItems::Image::CropImageBorder(
                             m_originalImage, static_cast<uint8_t>(m_cropBorderTolerance),
                             m_cropBorderColor) :
                         m_originalImage };

        if (m_removeGutterShadow)
            {
            img = GraphItems::Image::RemoveGutterShadow(
                img, static_cast<Wisteria::GutterSide>(m_gutterSide), m_gutterWidth / 100.0);
            }

        if (m_reduceBleedThrough)
            {
            img = GraphItems::Image::ReduceBleedThrough(
                img, static_cast<uint8_t>(m_bleedThroughWhitePoint));
            }

        if (m_binarize)
            {
            img = GraphItems::Image::BinarizeOtsu(img, m_binarizeThresholdAdjustment);
            }

        return img;
        }

    //----------------------------------------
    void ImageEffectDlg::UpdateCropBorderControls()
        {
        m_cropBorderToleranceLabel->Enable(m_cropImageBorder);
        m_cropBorderToleranceLabel->Refresh();
        m_cropBorderToleranceCtrl->Enable(m_cropImageBorder);
        m_cropBorderToleranceCtrl->Refresh();
        m_cropBorderColorLabel->Enable(m_cropImageBorder);
        m_cropBorderColorLabel->Refresh();
        m_cropBorderColorCtrl->Enable(m_cropImageBorder);
        m_cropBorderColorCtrl->Refresh();
        m_pickColorButton->Enable(m_cropImageBorder);
        m_pickColorButton->Refresh();
        }

    //----------------------------------------
    void ImageEffectDlg::UpdateSaveInplaceControls()
        {
        m_outputPathLabel->Enable(!m_saveInplace);
        m_outputPathLabel->Refresh();
        m_outputPathPicker->Enable(!m_saveInplace);
        m_outputPathPicker->Refresh();
        }

    //----------------------------------------
    void ImageEffectDlg::UpdateGutterShadowControls()
        {
        m_gutterSideLabel->Enable(m_removeGutterShadow);
        m_gutterSideLabel->Refresh();
        m_gutterSideChoice->Enable(m_removeGutterShadow);
        m_gutterSideChoice->Refresh();
        m_gutterWidthLabel->Enable(m_removeGutterShadow);
        m_gutterWidthLabel->Refresh();
        m_gutterWidthCtrl->Enable(m_removeGutterShadow);
        m_gutterWidthCtrl->Refresh();
        }

    //----------------------------------------
    void ImageEffectDlg::UpdateBleedThroughControls()
        {
        m_bleedThroughWhitePointLabel->Enable(m_reduceBleedThrough);
        m_bleedThroughWhitePointLabel->Refresh();
        m_bleedThroughWhitePointCtrl->Enable(m_reduceBleedThrough);
        m_bleedThroughWhitePointCtrl->Refresh();
        }

    //----------------------------------------
    void ImageEffectDlg::UpdateBinarizeControls()
        {
        m_binarizeThresholdLabel->Enable(m_binarize);
        m_binarizeThresholdLabel->Refresh();
        m_binarizeThresholdCtrl->Enable(m_binarize);
        m_binarizeThresholdCtrl->Refresh();
        }

    //----------------------------------------
    void ImageEffectDlg::SetSourceImage(const wxString& path)
        {
        if (path.empty() || !wxFileName::Exists(path))
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
        outputPath.SetName(sourcePath.GetName() + L"-" + wxDateTime::Now().Format(L"%Y%m%d%H%M%S"));
        return outputPath.GetFullPath();
        }
    } // namespace Wisteria::UI
