///////////////////////////////////////////////////////////////////////////////
// Name:        imagemergedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imagemergedlg.h"
#include "imageexportdlg.h"
#include "wx/valgen.h"
#include <wx/datetime.h>
#include <wx/quantize.h>
#include <wx/radiobox.h>
#include <wx/richmsgdlg.h>

namespace Wisteria::UI
    {
    //----------------------------------------
    void ImageMergeDlg::CreateControls(const wxArrayString& imgPaths)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        wxArrayString orientations;
        orientations.Add(_(L"Horizontally"));
        orientations.Add(_(L"Vertically"));
        auto* orientationsRadioBox =
            new wxRadioBox(this, wxID_ANY, _(L"Merge Images:"), wxDefaultPosition, wxDefaultSize,
                           orientations, 0, wxRA_SPECIFY_ROWS, wxGenericValidator(&m_orientRadio));
        orientationsRadioBox->SetSelection(0);
        mainSizer->Add(orientationsRadioBox, wxSizerFlags{}.Border());

        m_horizontalThumbsSizer = new wxStaticBoxSizer(wxHORIZONTAL, this);

        for (const auto& imgPath : imgPaths)
            {
            m_horizontalThumbsSizer->Add(
                new Thumbnail(m_horizontalThumbsSizer->GetStaticBox(),
                              Wisteria::GraphItems::Image::LoadFile(imgPath),
                              Wisteria::ClickMode::BrowseForImageFile, true));
            }

        AdjustThumbnailsHorizontally();

        mainSizer->Add(m_horizontalThumbsSizer, wxSizerFlags{ 1 }.Expand().Border());

        m_verticalThumbsSizer = new wxStaticBoxSizer(wxVERTICAL, this);

        for (const auto& imgPath : imgPaths)
            {
            m_verticalThumbsSizer->Add(new Thumbnail(m_verticalThumbsSizer->GetStaticBox(),
                                                     Wisteria::GraphItems::Image::LoadFile(imgPath),
                                                     Wisteria::ClickMode::BrowseForImageFile,
                                                     true));
            }

        AdjustThumbnailsVertically();

        mainSizer->Add(m_verticalThumbsSizer, wxSizerFlags{ 1 }.Expand().Border());

        mainSizer->Add(new wxStaticText(this, wxID_STATIC,
                                        _(L"Click any thumbnail to select a different image.\n"
                                          "Click OK to combine images into a new one.")),
                       wxSizerFlags{}.Expand().Border());

        auto* outputPathSizer = new wxBoxSizer(wxHORIZONTAL);
        outputPathSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Output image:")),
                             wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_outputPathPicker = new wxFilePickerCtrl(
            this, wxID_ANY, CreateDefaultOutputPath(imgPaths), _(L"Select Output Image"),
            GraphItems::Image::GetImageFileFilter(), wxDefaultPosition, wxDefaultSize,
            wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT | wxFLP_USE_TEXTCTRL);
        outputPathSizer->Add(m_outputPathPicker, wxSizerFlags{ 1 }.CenterVertical());
        mainSizer->Add(outputPathSizer, wxSizerFlags{}.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        m_horizontalThumbsSizer->Show(m_orientRadio == 0);
        m_verticalThumbsSizer->Show(m_orientRadio == 1);

        SetSizerAndFit(mainSizer);

        Bind(wxEVT_RADIOBOX,
             [this]([[maybe_unused]] const wxCommandEvent&)
             {
                 TransferDataFromWindow();
                 m_horizontalThumbsSizer->Show(m_orientRadio == 0);
                 m_verticalThumbsSizer->Show(m_orientRadio == 1);
                 if (m_orientRadio == 0)
                     {
                     AdjustThumbnailsHorizontally();
                     }
                 else
                     {
                     AdjustThumbnailsVertically();
                     }
             });
        Bind(wxEVT_THUMBNAIL_CHANGED,
             [this]([[maybe_unused]] const wxCommandEvent&)
             {
                 if (m_orientRadio == 0)
                     {
                     AdjustThumbnailsHorizontally();
                     }
                 else
                     {
                     AdjustThumbnailsVertically();
                     }
             });
        Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] const wxCommandEvent&)
            {
                std::vector<wxImage> images;

                const auto* thumbSize =
                    (m_orientRadio == 0) ? m_horizontalThumbsSizer : m_verticalThumbsSizer;

                for (const auto* sizerItem : thumbSize->GetChildren())
                    {
                    if (const auto* thumb{ dynamic_cast<const Thumbnail*>(sizerItem->GetWindow()) };
                        thumb != nullptr)
                        {
                        images.push_back(thumb->GetImage().GetOriginalImage());
                        }
                    }

                m_mergedFilePath = m_outputPathPicker->GetPath();
                if (m_mergedFilePath.empty())
                    {
                    wxMessageBox(_(L"Please select a path to save the image to."), _(L"Save"),
                                 wxOK | wxICON_EXCLAMATION);
                    return;
                    }
                if (wxFileName::Exists(m_mergedFilePath) &&
                    wxMessageBox(
                        wxString::Format(_(L"%s already exists.\nDo you want to replace it?"),
                                         m_mergedFilePath),
                        _(L"Save"), wxYES_NO | wxICON_QUESTION) != wxYES)
                    {
                    return;
                    }

                auto mergedImg = (m_orientRadio == 0) ?
                                     GraphItems::Image::StitchHorizontally(images) :
                                     GraphItems::Image::StitchVertically(images);

                mergedImg.SetOption(wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES);
                mergedImg.SetOption(wxIMAGE_OPTION_RESOLUTIONX,
                                    Settings::GetImageResolutionDPI().GetWidth());
                mergedImg.SetOption(wxIMAGE_OPTION_RESOLUTIONY,
                                    Settings::GetImageResolutionDPI().GetHeight());

                wxString outputExt{ wxFileName{ m_mergedFilePath }.GetExt() };
                const wxBitmapType outputImageType =
                    GraphItems::Image::GetImageFileTypeFromExtension(outputExt);
                if (outputImageType == wxBITMAP_TYPE_TIF)
                    {
                    mergedImg.SetOption(wxIMAGE_OPTION_COMPRESSION,
                                        static_cast<int>(TiffCompression::CompressionNone));
                    }
                else if (outputImageType == wxBITMAP_TYPE_JPEG)
                    {
                    mergedImg.SetOption(wxIMAGE_OPTION_QUALITY, 100);
                    }
                else if (outputImageType == wxBITMAP_TYPE_PNG)
                    {
                    mergedImg.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
                    }
                else if (outputImageType == wxBITMAP_TYPE_GIF)
                    {
                    wxQuantize::Quantize(mergedImg, mergedImg, 256);
                    mergedImg.ConvertAlphaToMask();
                    }

                if (!mergedImg.SaveFile(m_mergedFilePath))
                    {
                    wxMessageBox(_(L"Unable to save merged image."), _(L"Save"), wxOK);
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
    void ImageMergeDlg::AdjustThumbnailsHorizontally()
        {
        const wxWindowUpdateLocker noUpdates(m_horizontalThumbsSizer->GetStaticBox());
        int maxHeight{ 0 };
        int totalWidth{ 0 };
        for (const auto* sizerItem : m_horizontalThumbsSizer->GetChildren())
            {
            if (const auto* thumb{ dynamic_cast<const Thumbnail*>(sizerItem->GetWindow()) };
                thumb != nullptr)
                {
                maxHeight = std::max(thumb->GetImage().GetOriginalImage().GetHeight(), maxHeight);
                totalWidth += thumb->GetImage().GetOriginalImage().GetWidth();
                }
            }
        const double scale{ std::min(safe_divide<double>(FromDIP(512), maxHeight),
                                     safe_divide<double>(FromDIP(1000), totalWidth)) };
        for (auto* sizerItem : m_horizontalThumbsSizer->GetChildren())
            {
            if (auto* thumb{ dynamic_cast<Thumbnail*>(sizerItem->GetWindow()) }; thumb != nullptr)
                {
                const wxSize originalSize{ thumb->GetImage().GetOriginalImage().GetSize() };
                sizerItem->SetMinSize(wxSize{
                    std::max(FromDIP(32), static_cast<int>(originalSize.GetWidth() * scale)),
                    std::max(FromDIP(32), static_cast<int>(originalSize.GetHeight() * scale)) });
                }
            }

        if (GetSizer() != nullptr)
            {
            GetSizer()->Fit(this);
            GetSizer()->SetSizeHints(this);
            }
        }

    //----------------------------------------
    void ImageMergeDlg::AdjustThumbnailsVertically()
        {
        const wxWindowUpdateLocker noUpdates(m_verticalThumbsSizer->GetStaticBox());
        int maxWidth{ 0 };
        int totalHeight{ 0 };
        for (const auto* sizerItem : m_verticalThumbsSizer->GetChildren())
            {
            if (const auto* thumb{ dynamic_cast<const Thumbnail*>(sizerItem->GetWindow()) };
                thumb != nullptr)
                {
                maxWidth = std::max(thumb->GetImage().GetOriginalImage().GetWidth(), maxWidth);
                totalHeight += thumb->GetImage().GetOriginalImage().GetHeight();
                }
            }
        const double scale{ std::min(safe_divide<double>(FromDIP(512), maxWidth),
                                     safe_divide<double>(FromDIP(1000), totalHeight)) };
        for (auto* sizerItem : m_verticalThumbsSizer->GetChildren())
            {
            if (auto* thumb{ dynamic_cast<Thumbnail*>(sizerItem->GetWindow()) }; thumb != nullptr)
                {
                const wxSize originalSize{ thumb->GetImage().GetOriginalImage().GetSize() };
                sizerItem->SetMinSize(wxSize{
                    std::max(FromDIP(32), static_cast<int>(originalSize.GetWidth() * scale)),
                    std::max(FromDIP(32), static_cast<int>(originalSize.GetHeight() * scale)) });
                }
            }

        if (GetSizer() != nullptr)
            {
            GetSizer()->Fit(this);
            GetSizer()->SetSizeHints(this);
            }
        }

    //----------------------------------------
    wxString ImageMergeDlg::CreateDefaultOutputPath(const wxArrayString& imgPaths)
        {
        if (imgPaths.empty())
            {
            return wxString{};
            }

        wxString combinedName;
        for (const auto& imgPath : imgPaths)
            {
            combinedName += wxFileName{ imgPath }.GetName();
            }

        wxFileName outputPath{ imgPaths[0] };
        outputPath.SetName(combinedName);
        if (outputPath.Exists())
            {
            outputPath.SetName(combinedName + L"-" + wxDateTime::Now().Format(L"%Y%m%d%H%M%S"));
            }

        return outputPath.GetFullPath();
        }
    } // namespace Wisteria::UI
