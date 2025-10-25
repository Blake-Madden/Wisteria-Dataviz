///////////////////////////////////////////////////////////////////////////////
// Name:        imagemergedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imagemergedlg.h"
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

        m_baseImagePath = wxFileName{ (!imgPaths.empty() ? wxFileName(imgPaths[0]) : wxString{}) };

        wxString imageNames;
        for (const auto& imgPath : imgPaths)
            {
            imageNames += wxFileName{ imgPath }.GetName();
            m_horizontalThumbsSizer->Add(
                new Thumbnail(m_horizontalThumbsSizer->GetStaticBox(),
                              Wisteria::GraphItems::Image::LoadFile(imgPath),
                              Wisteria::ClickMode::BrowseForImageFile, true));
            }
        m_baseImagePath.SetName(imageNames);

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

                wxFileDialog fd(this, _(L"Select Output Image"), m_baseImagePath.GetPath(),
                                m_baseImagePath.GetFullName(),
                                GraphItems::Image::GetImageFileFilter(),
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_PREVIEW);
                if (fd.ShowModal() != wxID_OK)
                    {
                    return;
                    }

                m_mergedFilePath = fd.GetPath();

                if (m_orientRadio == 0)
                    {
                    if (!GraphItems::Image::StitchHorizontally(images).SaveFile(m_mergedFilePath))
                        {
                        wxMessageBox(_(L"Unable to save merged image."), _(L"Save"), wxOK);
                        }
                    }
                else
                    {
                    if (!GraphItems::Image::StitchVertically(images).SaveFile(m_mergedFilePath))
                        {
                        wxMessageBox(_(L"Unable to save merged image."), _(L"Save"), wxOK);
                        }
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
        wxWindowUpdateLocker noUpdates(m_horizontalThumbsSizer->GetStaticBox());
        int maxHeight{ 0 };
        for (const auto* sizerItem : m_horizontalThumbsSizer->GetChildren())
            {
            if (const auto* thumb{ dynamic_cast<const Thumbnail*>(sizerItem->GetWindow()) };
                thumb != nullptr)
                {
                maxHeight = std::max(thumb->GetImage().GetOriginalImage().GetHeight(), maxHeight);
                }
            }
        for (auto* sizerItem : m_horizontalThumbsSizer->GetChildren())
            {
            if (auto* thumb{ dynamic_cast<Thumbnail*>(sizerItem->GetWindow()) }; thumb != nullptr)
                {
                const double percentOfMaxHeight{ safe_divide<double>(
                    thumb->GetImage().GetOriginalImage().GetHeight(), maxHeight) };
                wxSize bestSize = thumb->GetImage().GetBestSize(
                    wxSize{ FromDIP(512), static_cast<int>(percentOfMaxHeight * FromDIP(512)) });
                sizerItem->SetMinSize(bestSize);
                }
            }

        if (GetSizer() != nullptr)
            {
            GetSizer()->Fit(this);
            }
        }

    //----------------------------------------
    void ImageMergeDlg::AdjustThumbnailsVertically()
        {
        wxWindowUpdateLocker noUpdates(m_verticalThumbsSizer->GetStaticBox());
        int maxWidth{ 0 };
        for (const auto* sizerItem : m_verticalThumbsSizer->GetChildren())
            {
            if (const auto* thumb{ dynamic_cast<const Thumbnail*>(sizerItem->GetWindow()) };
                thumb != nullptr)
                {
                maxWidth = std::max(thumb->GetImage().GetOriginalImage().GetWidth(), maxWidth);
                }
            }
        for (auto* sizerItem : m_verticalThumbsSizer->GetChildren())
            {
            if (auto* thumb{ dynamic_cast<Thumbnail*>(sizerItem->GetWindow()) }; thumb != nullptr)
                {
                const double percentOfMaxWidth{ safe_divide<double>(
                    thumb->GetImage().GetOriginalImage().GetWidth(), maxWidth) };
                wxSize bestSize = thumb->GetImage().GetBestSize(
                    wxSize{ static_cast<int>(percentOfMaxWidth * FromDIP(512)), FromDIP(512) });
                sizerItem->SetMinSize(bestSize);
                }
            }

        if (GetSizer() != nullptr)
            {
            GetSizer()->Fit(this);
            }
        }
    } // namespace Wisteria::UI
