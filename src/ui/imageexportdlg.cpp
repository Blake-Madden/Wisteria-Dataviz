/////////////////////////////////////////////////////////////////////////////
// Name:        imageexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imageexportdlg.h"

using namespace Wisteria::UI;

void ImageExportDlg::OnOptionsChanged([[maybe_unused]] wxCommandEvent& event)
    {
    if (!m_previewThumbnail || !m_originalBitmap.IsOk())
        { return; }

    TransferDataFromWindow();

    wxImage img(m_originalBitmap.ConvertToImage());

    if (m_options.m_mode == static_cast<decltype(m_options.m_mode)>(ImageExportOptions::ColorMode::Grayscale))
        { img = img.ConvertToGreyscale(); }

    m_previewThumbnail->SetBitmap(wxBitmap(img));
    }

void ImageExportDlg::OnSizeChanged(wxSpinEvent& event)
    {
    std::pair<double,double> imgSize(m_options.m_imageSize.x, m_options.m_imageSize.y);
    TransferDataFromWindow();

    if (event.GetId() == IMAGE_WIDTH_ID)
        {
        m_options.m_imageSize.y = geometry::calculate_rescale_height(
                                    std::make_pair(imgSize.first, imgSize.second),
                                    m_options.m_imageSize.x);
        }
    else
        {
        m_options.m_imageSize.x = geometry::calculate_rescale_width(
            std::make_pair(imgSize.first, imgSize.second),
            m_options.m_imageSize.y);
        }

    TransferDataToWindow();
    }

/// Creation
bool ImageExportDlg::Create(wxWindow* parent,
                            const wxBitmapType bitmapType, wxWindowID id /*= wxID_ANY*/,
                            const wxString& caption /*= _(L"Image Export Options")*/,
                            const wxPoint& pos /*= wxDefaultPosition*/,
                            const wxSize& size /*= wxDefaultSize*/,
                            long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN*/)
    {
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls(bitmapType);

    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ImageExportDlg::OnHelpClicked, this, wxID_HELP);
    Bind(wxEVT_HELP, &ImageExportDlg::OnContextHelp, this);
    Bind(wxEVT_BUTTON, &ImageExportDlg::OnOK, this, wxID_OK);
    Bind(wxEVT_SPINCTRL, &ImageExportDlg::OnSizeChanged, this);
    Bind(wxEVT_RADIOBOX, &ImageExportDlg::OnOptionsChanged, this, ImageExportDlg::COLOR_MODE_COMBO_ID);

    Centre();
    return true;
    }

/// Creates the controls and sizers
//-------------------------------------------------------------
void ImageExportDlg::CreateControls(const wxBitmapType bitmapType)
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* column1Sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* column2Sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);
    controlsSizer->Add(column1Sizer);
    controlsSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
    controlsSizer->Add(column2Sizer);
    mainSizer->Add(controlsSizer, 0, wxEXPAND|wxALIGN_TOP|wxALL, wxSizerFlags::GetDefaultBorder());

    wxStaticBoxSizer* imageSizeSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Image Size"));
    auto imageSizeInfoSizer = new wxGridSizer(2, 2, wxSize(wxSizerFlags::GetDefaultBorder(),wxSizerFlags::GetDefaultBorder()));
    imageSizeSizer->Add(imageSizeInfoSizer,1,wxEXPAND);

    wxStaticText* widthLabel = new wxStaticText(this, wxID_STATIC, _(L"Width:"), wxDefaultPosition, wxDefaultSize);
    imageSizeInfoSizer->Add(widthLabel, 0, wxALIGN_CENTER_VERTICAL);
    wxSpinCtrl* widthCtrl = new wxSpinCtrl(this, IMAGE_WIDTH_ID,
        wxString::Format(L"%d", m_options.m_imageSize.GetWidth()),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 10, 10000);
    widthCtrl->SetValidator(wxGenericValidator(&m_options.m_imageSize.x));
    imageSizeInfoSizer->Add(widthCtrl, 0);

    wxStaticText* heightLabel = new wxStaticText(this, wxID_STATIC, _(L"Height:"), wxDefaultPosition, wxDefaultSize);
    imageSizeInfoSizer->Add(heightLabel, 0, wxALIGN_CENTER_VERTICAL);
    wxSpinCtrl* heightCtrl = new wxSpinCtrl(this, IMAGE_HEIGHT_ID,
        wxString::Format(L"%d", m_options.m_imageSize.GetHeight()),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 10, 10000);
    heightCtrl->SetValidator(wxGenericValidator(&m_options.m_imageSize.y));
    imageSizeInfoSizer->Add(heightCtrl);
    column1Sizer->Add(imageSizeSizer, 0, wxEXPAND);

    if (bitmapType != wxBITMAP_TYPE_ANY) // unknown/non-image formats (e.g., SVG) won't use these options
        {
        wxArrayString colorModes;
        colorModes.Add(_(L"&RGB (Color)"));
        colorModes.Add(_(L"&Grayscale"));
        wxRadioBox* colorModesRadioBox = new wxRadioBox(this, COLOR_MODE_COMBO_ID, _(L"Color Mode"),
            wxDefaultPosition, wxDefaultSize, colorModes, 0, wxRA_SPECIFY_ROWS, wxGenericValidator(&m_options.m_mode) );
        column1Sizer->Add(colorModesRadioBox, 0, wxEXPAND);
        column1Sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
        }

    if (bitmapType == wxBITMAP_TYPE_TIF)
        {
        wxStaticBox* tiffBox = new wxStaticBox(this, wxID_ANY, _(L"TIFF options:"));
        wxStaticBoxSizer* tiffOptionsBoxSizer = new wxStaticBoxSizer(tiffBox, wxVERTICAL);
        column1Sizer->Add(tiffOptionsBoxSizer, 0, wxEXPAND);

        wxBoxSizer* compressionSizer = new wxBoxSizer(wxHORIZONTAL);
        tiffOptionsBoxSizer->Add(compressionSizer, 0, wxALIGN_LEFT|wxALL, wxSizerFlags::GetDefaultBorder());
        wxStaticText* compressionLabel = new wxStaticText(this, wxID_STATIC, _(L"Compression:"), wxDefaultPosition, wxDefaultSize, 0);
        compressionSizer->Add(compressionLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, wxSizerFlags::GetDefaultBorder());

        wxArrayString compressionChoices;
        compressionChoices.Add(_(L"None"));
        compressionChoices.Add(_DT(L"Lempel-Ziv & Welch", DTExplanation::ProperNoun));
        compressionChoices.Add(_DT(L"JPEG"));
        compressionChoices.Add(_(L"Deflate"));
        m_tiffCompressionCombo = new wxComboBox(this, wxID_ANY, wxEmptyString,
                                            wxDefaultPosition, wxDefaultSize, compressionChoices,
                                            wxCB_DROPDOWN|wxCB_READONLY);
        m_tiffCompressionCombo->SetSelection((m_options.m_tiffCompression == TiffCompression::CompressionNone) ?
                                                0 : (m_options.m_tiffCompression == TiffCompression::CompressionLZW) ?
                                1 : (m_options.m_tiffCompression == TiffCompression::CompressionJPEG) ?
                                2 : (m_options.m_tiffCompression == TiffCompression::CompressionDeflate) ?
                                3 : 0);
        compressionSizer->Add(m_tiffCompressionCombo);
        }

    if (m_originalBitmap.IsOk())
        {
        wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_previewThumbnail = new Thumbnail(this, m_originalBitmap, Thumbnail::ClickMode::DoNothing,
            false, wxID_ANY, wxDefaultPosition, wxSize(128,128));
        previewSizer->Add(m_previewThumbnail);
        column2Sizer->Add(previewSizer);
        }

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL|wxHELP), 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);
    }

//-------------------------------------------------------------
void ImageExportDlg::OnOK([[maybe_unused]] wxCommandEvent& event)
    {
    TransferDataFromWindow();

    if (m_tiffCompressionCombo)
        {
        switch (m_tiffCompressionCombo->GetSelection())
            {
        case 0:
            m_options.m_tiffCompression = TiffCompression::CompressionNone;
            break;
        case 1:
            m_options.m_tiffCompression = TiffCompression::CompressionLZW;
            break;
        case 2:
            m_options.m_tiffCompression = TiffCompression::CompressionJPEG;
            break;
        case 3:
            m_options.m_tiffCompression = TiffCompression::CompressionDeflate;
            break;
        default:
            m_options.m_tiffCompression = TiffCompression::CompressionNone;
            }
        }
    else
        { m_options.m_tiffCompression = TiffCompression::CompressionNone; }

    if (IsModal())
        { EndModal(wxID_OK); }
    else
        { Show(false); }
    }
