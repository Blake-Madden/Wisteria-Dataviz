/////////////////////////////////////////////////////////////////////////////
// Name:        imageexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "imageexportdlg.h"
#include "../../base/canvas.h"

using namespace Wisteria::UI;

//------------------------------------------------------
void ImageExportDlg::OnOptionsChanged([[maybe_unused]] wxCommandEvent& event)
    {
    if (!m_previewThumbnail || !m_originalBitmap.IsOk())
        {
        return;
        }

    TransferDataFromWindow();

    wxImage img(m_originalBitmap.ConvertToImage());

    if (m_options.m_mode ==
        static_cast<decltype(m_options.m_mode)>(ImageExportOptions::ColorMode::Grayscale))
        {
        img = img.ConvertToGreyscale();
        }

    m_previewThumbnail->SetBitmap(wxBitmap(img));
    }

//------------------------------------------------------
void ImageExportDlg::OnSizeChanged(const wxSpinEvent& event)
    {
    std::pair<double, double> imgSize(m_options.m_imageSize.x, m_options.m_imageSize.y);
    TransferDataFromWindow();

    if (event.GetId() == ControlIDs::IMAGE_WIDTH_ID)
        {
        m_options.m_imageSize.y = geometry::rescaled_height(
            std::make_pair(imgSize.first, imgSize.second), m_options.m_imageSize.x);
        }
    else
        {
        m_options.m_imageSize.x = geometry::rescaled_width(
            std::make_pair(imgSize.first, imgSize.second), m_options.m_imageSize.y);
        }

    TransferDataToWindow();
    }

//------------------------------------------------------
bool ImageExportDlg::Create(wxWindow* parent, const wxBitmapType bitmapType,
                            wxWindowID id /*= wxID_ANY*/,
                            const wxString& caption /*= _(L"Image Export Options")*/,
                            const wxPoint& pos /*= wxDefaultPosition*/,
                            const wxSize& size /*= wxDefaultSize*/,
                            long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN*/)
    {
    SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls(bitmapType);

    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ImageExportDlg::OnHelpClicked, this, wxID_HELP);
    Bind(wxEVT_HELP, &ImageExportDlg::OnContextHelp, this);
    Bind(wxEVT_BUTTON, &ImageExportDlg::OnOK, this, wxID_OK);
    Bind(wxEVT_SPINCTRL, &ImageExportDlg::OnSizeChanged, this);
    Bind(wxEVT_RADIOBOX, &ImageExportDlg::OnOptionsChanged, this, ControlIDs::COLOR_MODE_COMBO_ID);

    Centre();
    return true;
    }

/// Creates the controls and sizers
//------------------------------------------------------
void ImageExportDlg::CreateControls(const wxBitmapType bitmapType)
    {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    auto* column1Sizer = new wxBoxSizer(wxVERTICAL);
    auto* column2Sizer = new wxBoxSizer(wxVERTICAL);
    auto* controlsSizer = new wxBoxSizer(wxHORIZONTAL);
    controlsSizer->Add(column1Sizer);
    controlsSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
    controlsSizer->Add(column2Sizer);
    mainSizer->Add(controlsSizer, wxSizerFlags{}.Expand().Border().Top());

    auto* imageSizeSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Image Size"));
    auto* imageSizeInfoSizer = new wxGridSizer(
        2, 2, wxSize(wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder()));
    imageSizeSizer->Add(imageSizeInfoSizer, wxSizerFlags{ 1 }.Expand());

    auto* widthLabel = new wxStaticText(imageSizeSizer->GetStaticBox(), wxID_STATIC, _(L"Width:"),
                                        wxDefaultPosition, wxDefaultSize);
    imageSizeInfoSizer->Add(widthLabel, wxSizerFlags{}.CenterVertical());
    auto* widthCtrl =
        new wxSpinCtrl(imageSizeSizer->GetStaticBox(), ControlIDs::IMAGE_WIDTH_ID,
                       std::to_wstring(m_options.m_imageSize.GetWidth()), wxDefaultPosition,
                       wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
    widthCtrl->SetValidator(wxGenericValidator(&m_options.m_imageSize.x));
    imageSizeInfoSizer->Add(widthCtrl, 0);

    auto* heightLabel = new wxStaticText(imageSizeSizer->GetStaticBox(), wxID_STATIC, _(L"Height:"),
                                         wxDefaultPosition, wxDefaultSize);
    imageSizeInfoSizer->Add(heightLabel, wxSizerFlags{}.CenterVertical());
    auto* heightCtrl =
        new wxSpinCtrl(imageSizeSizer->GetStaticBox(), ControlIDs::IMAGE_HEIGHT_ID,
                       std::to_wstring(m_options.m_imageSize.GetHeight()), wxDefaultPosition,
                       wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
    heightCtrl->SetValidator(wxGenericValidator(&m_options.m_imageSize.y));
    imageSizeInfoSizer->Add(heightCtrl);
    column1Sizer->Add(imageSizeSizer, wxSizerFlags{}.Expand());

    // unknown/non-image formats (e.g., SVG) won't use these options
    if (bitmapType != wxBITMAP_TYPE_ANY)
        {
        wxArrayString colorModes;
        colorModes.Add(_(L"&RGB (Color)"));
        colorModes.Add(_(L"&Grayscale"));
        auto* colorModesRadioBox = new wxRadioBox(
            this, ControlIDs::COLOR_MODE_COMBO_ID, _(L"Color Mode"), wxDefaultPosition,
            wxDefaultSize, colorModes, 0, wxRA_SPECIFY_ROWS, wxGenericValidator(&m_options.m_mode));
        column1Sizer->Add(colorModesRadioBox, wxSizerFlags{}.Expand());
        column1Sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
        }

    if (bitmapType == wxBITMAP_TYPE_TIF)
        {
        auto* tiffBox = new wxStaticBox(this, wxID_ANY, _(L"TIFF options:"));
        auto* tiffOptionsBoxSizer = new wxStaticBoxSizer(tiffBox, wxVERTICAL);
        column1Sizer->Add(tiffOptionsBoxSizer, wxSizerFlags{}.Expand());

        auto* compressionSizer = new wxBoxSizer(wxHORIZONTAL);
        tiffOptionsBoxSizer->Add(compressionSizer, wxSizerFlags{}.Border());
        auto* compressionLabel =
            new wxStaticText(tiffOptionsBoxSizer->GetStaticBox(), wxID_STATIC, _(L"Compression:"),
                             wxDefaultPosition, wxDefaultSize, 0);
        compressionSizer->Add(compressionLabel, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));

        wxArrayString compressionChoices;
        compressionChoices.Add(_(L"None"));
        compressionChoices.Add(_DT(L"Lempel-Ziv & Welch", DTExplanation::ProperNoun));
        compressionChoices.Add(_DT(L"JPEG"));
        compressionChoices.Add(_(L"Deflate"));
        m_tiffCompressionCombo = new wxComboBox(tiffOptionsBoxSizer->GetStaticBox(), wxID_ANY,
                                                wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                                compressionChoices, wxCB_DROPDOWN | wxCB_READONLY);
        m_tiffCompressionCombo->SetSelection(
            (m_options.m_tiffCompression == TiffCompression::CompressionNone)    ? 0 :
            (m_options.m_tiffCompression == TiffCompression::CompressionLZW)     ? 1 :
            (m_options.m_tiffCompression == TiffCompression::CompressionJPEG)    ? 2 :
            (m_options.m_tiffCompression == TiffCompression::CompressionDeflate) ? 3 :
                                                                                   0);
        compressionSizer->Add(m_tiffCompressionCombo);
        }

    if (m_originalBitmap.IsOk())
        {
        auto* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        m_previewThumbnail =
            new Thumbnail(previewSizer->GetStaticBox(), m_originalBitmap, ClickMode::DoNothing,
                          false, wxID_ANY, wxDefaultPosition, FromDIP(wxSize{ 512, 512 }));
        previewSizer->Add(m_previewThumbnail);
        column2Sizer->Add(previewSizer);
        }

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                   wxSizerFlags{}.Expand().Border());

    SetSizerAndFit(mainSizer);
    }

//------------------------------------------------------
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
        {
        m_options.m_tiffCompression = TiffCompression::CompressionNone;
        }

    if (IsModal())
        {
        EndModal(wxID_OK);
        }
    else
        {
        Show(false);
        }
    }
