///////////////////////////////////////////////////////////////////////////////
// Name:        image.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "image.h"
#include "polygon.h"

using namespace Wisteria::Colors;

namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------
    wxSize Image::GetSVGSize(const wxString& filePath)
        {
        NSVGimage* image{ nullptr };
        image = nsvgParseFromFile(filePath.c_str(), "px", 96.0f);
        if (image == nullptr)
            { return wxDefaultSize; }
        wxSize sz{ static_cast<int>(image->width) ,
                   static_cast<int>(image->height) };

        nsvgDelete(image);

        return sz;
        }

    //----------------------------------------------------------
    wxImage Image::CropImageToRect(const wxImage& img, const wxRect rect)
        {
        wxImage scaledCommonImg;
        if (img.IsOk() &&
            img.GetWidth() >= rect.GetSize().GetWidth() &&
            img.GetHeight() >= rect.GetSize().GetHeight())
            {
            const auto heightRatio = safe_divide<double>(
                img.GetHeight(),
                rect.GetSize().GetHeight());
            const auto weightRatio = safe_divide<double>(
                img.GetWidth(),
                rect.GetSize().GetWidth());
            // height is proportionally larger, so fit by width and then crop
            // the height evenly on the top and bottom
            if (heightRatio >= weightRatio)
                {
                const auto scaledHeight = geometry::calculate_rescale_height(
                    std::make_pair(img.GetWidth(),
                                   img.GetHeight()),
                    rect.GetSize().GetWidth());
                scaledCommonImg = img.Scale(rect.GetSize().GetWidth(), scaledHeight);

                const auto crop = scaledCommonImg.GetHeight() -
                    rect.GetSize().GetHeight();
                scaledCommonImg = scaledCommonImg.GetSubImage(
                    wxRect(wxPoint(0, std::floor(safe_divide<double>(crop, 2))),
                           rect.GetSize()));
                wxASSERT_MSG((scaledCommonImg.GetSize().GetHeight() >=
                    rect.GetSize().GetHeight()),
                    wxString::Format(L"Common image not scaled height-wise large enough! %d vs %d",
                        scaledCommonImg.GetSize().GetHeight(),
                        rect.GetSize().GetHeight()));
                }
            else
                {
                const auto scaledWidth = geometry::calculate_rescale_width(
                    std::make_pair(img.GetWidth(),
                                   img.GetHeight()),
                    rect.GetSize().GetHeight());
                scaledCommonImg = img.Scale(scaledWidth, rect.GetSize().GetHeight());

                const auto crop = scaledCommonImg.GetWidth() -
                                  rect.GetSize().GetWidth();
                scaledCommonImg = scaledCommonImg.GetSubImage(
                    wxRect(wxPoint(std::floor(safe_divide<double>(crop, 2)), 0),
                           rect.GetSize()));
                wxASSERT_MSG((scaledCommonImg.GetSize().GetWidth() >=
                              rect.GetSize().GetWidth()),
                             wxString::Format(L"Common image not scaled width-wise large enough! %d vs %d",
                                              scaledCommonImg.GetSize().GetWidth(),
                                              rect.GetSize().GetWidth()));
                }
            }
        return scaledCommonImg;
        }

    //----------------------------------------------------------
    wxImage Image::StitchHorizontally(const std::initializer_list<wxImage>& images)
        {
        if (images.size() == 0)
            { return wxNullImage; }
        const int imgWidth = std::accumulate(images.begin(), images.end(),
            0.0f,
            [](const auto initVal, const auto& img) noexcept
              { return initVal + img.GetWidth(); });
        const auto maxHeightImg = std::max_element(images.begin(), images.end(),
            [](const auto& img1, const auto& img2) noexcept
              { return img1.GetHeight() < img2.GetHeight(); });
        wxBitmap bmp(imgWidth, maxHeightImg->GetHeight());

        wxMemoryDC memDC(bmp);
        memDC.SetBrush(*wxWHITE);
        memDC.Clear();

        int currentX{ 0 };
        for (const auto& img : images)
            {
            memDC.DrawBitmap(wxBitmap(img),
                wxPoint(currentX, (memDC.GetSize().GetHeight()-img.GetHeight())/2));
            currentX += img.GetWidth();
            }
        memDC.SelectObject(wxNullBitmap);

        return bmp.ConvertToImage();
        }

    //----------------------------------------------------------
    wxImage Image::StitchVertically(const std::initializer_list<wxImage>& images)
        {
        if (images.size() == 0)
            { return wxNullImage; }
        const int imgHeight = std::accumulate(images.begin(), images.end(),
            0.0f,
            [](const auto initVal, const auto& img) noexcept
              { return initVal + img.GetHeight(); });
        const auto maxWidthImg = std::max_element(images.begin(), images.end(),
            [](const auto& img1, const auto& img2) noexcept
              { return img1.GetWidth() < img2.GetWidth(); });
        wxBitmap bmp(maxWidthImg->GetWidth(), imgHeight);

        wxMemoryDC memDC(bmp);
        memDC.SetBrush(*wxWHITE);
        memDC.Clear();

        int currentY{ 0 };
        for (const auto& img : images)
            {
            memDC.DrawBitmap(wxBitmap(img),
                wxPoint((memDC.GetSize().GetWidth()-img.GetWidth())/2, currentY));
            currentY += img.GetHeight();
            }
        memDC.SelectObject(wxNullBitmap);

        return bmp.ConvertToImage();
        }

    //-------------------------------------------
    void Image::SetOpacity(wxImage& image, const uint8_t opacity,
                           const wxColour colorToPreserve)
        {
        if (!image.IsOk())
            { return; }
        if (!colorToPreserve.IsOk())
            {
            SetOpacity(image, opacity, true);
            return;
            }
        const long pixelCount = image.GetWidth()*image.GetHeight();

        const auto redChannel = colorToPreserve.GetRed();
        const auto greenChannel = colorToPreserve.GetGreen();
        const auto blueChannel = colorToPreserve.GetBlue();

        if (!image.HasAlpha())
            { image.InitAlpha(); }
        if (image.HasAlpha())
            {
            auto alphaData = image.GetAlpha();
            const auto rgbData = image.GetData();
            if (alphaData)
                {
                for (long i = 0; i < pixelCount; ++i)
                    {
                    if (!(rgbData[i*3] == redChannel &&
                          rgbData[(i*3) +2] == greenChannel &&
                          rgbData[(i*3) + 2] == blueChannel))
                        { alphaData[i] = opacity; }
                    }
                }
            }
        }

    //-------------------------------------------
    void Image::SetOpacity(wxImage& image, const uint8_t opacity,
                           const bool preserveTransparentPixels)
        {
        if (!image.IsOk())
            { return; }
        const long pixelCount = image.GetWidth()*image.GetHeight();

        if (!image.HasAlpha())
            { image.InitAlpha(); }
        if (image.HasAlpha())
            {
            if (preserveTransparentPixels)
                {
                unsigned char* alphaData = image.GetAlpha();
                if (alphaData)
                    {
                    for (long i = 0; i < pixelCount; ++i)
                        {
                        if (alphaData[i] != 0)
                            { alphaData[i] = opacity; }
                        }
                    }
                }
            else
                {
                // must use malloc (not new) when setting alpha channel
                unsigned char* alphaData = static_cast<unsigned char*>(malloc(pixelCount));
                if (alphaData)
                    {
                    std::memset(alphaData, opacity, pixelCount);
                    image.SetAlpha(alphaData);
                    }
                }
            }
        }

    //-------------------------------------------
    wxImage Image::ChangeColor(const wxImage& image, const wxColour srcColor,
                               const wxColour destColor)
        {
        if (!image.IsOk())
            { return wxNullImage; }

        wxImage img{ image };
        const size_t pixelRGBCount = static_cast<size_t>(img.GetWidth()*img.GetHeight())*3;
        unsigned char* const rgbData = img.GetData();
        if (rgbData)
            {
            for (size_t i = 0; i < pixelRGBCount; i += 3)
                {
                if (rgbData[i] == srcColor.Red() &&
                    rgbData[i+1] == srcColor.Green() &&
                    rgbData[i+2] == srcColor.Blue())
                    {
                    rgbData[i] = destColor.Red();
                    rgbData[i+1] = destColor.Green();
                    rgbData[i+2] = destColor.Blue();
                    }
                }
            }
        return img;
        }

    //-------------------------------------------
    wxImage Image::CreateSilhouette(const wxImage& image, const bool opaque /*= true*/)
        {
        if (!image.IsOk())
            { return wxNullImage; }
        wxImage Silhouette = image.ConvertToMono(0,0,0);
        SetColorTransparent(Silhouette, *wxWHITE);
        if (!opaque)
            {
            Silhouette = ChangeColor(Silhouette, *wxBLACK,
                                     ColorBrewer::GetColor(Color::LightGray));
            }
        return Silhouette;
        }

    //-------------------------------------------
    void Image::SetColorTransparent(wxImage& image, const wxColour color)
        {
        if (!image.IsOk())
            { return; }
        if (!image.HasAlpha())
            { image.InitAlpha(); }
        if (image.HasAlpha())
            {
            const size_t pixelRGBCount = static_cast<size_t>(image.GetWidth()*image.GetHeight())*3;
            const unsigned char* rgbData = image.GetData();
            unsigned char* alphaData = image.GetAlpha();
            if (rgbData && alphaData)
                {
                for (size_t i = 0, j = 0; i < pixelRGBCount; i += 3, ++j)
                    {
                    if (rgbData[i] == color.Red() &&
                        rgbData[i+1] == color.Green() &&
                        rgbData[i+2] == color.Blue())
                        { alphaData[j] = 0; }
                    }
                }
            }
        }

    //-------------------------------------------
    wxImage Image::CreateGlassEffect(const wxSize fillSize, const wxColour color,
                                     const Orientation direction)
        {
        wxBitmap background(fillSize);
        wxMemoryDC memDC(background);
        // fill with the color
        memDC.GradientFillLinear(wxRect(fillSize), color, color.ChangeLightness(140),
                                 (direction == Orientation::Vertical) ? wxSOUTH : wxEAST);
        // create a shiny overlay
        memDC.GradientFillLinear(wxRect(0, 0,
                                    (direction == Orientation::Vertical) ?
                                        fillSize.GetWidth() : fillSize.GetWidth()*.25,
                                    (direction == Orientation::Vertical) ?
                                        fillSize.GetHeight()*.25 : fillSize.GetHeight()),
                                 color.ChangeLightness(115),color.ChangeLightness(155),
                                 (direction == Orientation::Vertical) ? wxSOUTH : wxEAST);
        memDC.SelectObject(wxNullBitmap);

        return background.ConvertToImage();
        }

    //-------------------------------------------
    void Image::SetOpacity(wxBitmap& bmp, const uint8_t opacity,
                           const bool preserveTransparentPixels /*= false*/)
        {
        if (!bmp.IsOk())
            { return; }
        wxImage bkImage = bmp.ConvertToImage();
        SetOpacity(bkImage, opacity, preserveTransparentPixels);

        bmp = wxBitmap(bkImage);
        wxASSERT_LEVEL_2(bmp.IsOk());
        }

    //-------------------------------------------
    void Image::SetOpacity(wxBitmap& bmp, const uint8_t opacity,
                           const wxColour colorToPreserve)
        {
        if (!bmp.IsOk())
            { return; }
        wxImage bkImage = bmp.ConvertToImage();
        SetOpacity(bkImage, opacity, colorToPreserve);

        bmp = wxBitmap(bkImage);
        wxASSERT_LEVEL_2(bmp.IsOk());
        }

    //-------------------------------------------
    wxImage Image::CreateStippledImage(wxImage stipple, const wxSize fillSize,
            const Orientation direction, const bool includeShadow, const wxCoord shadowSize)
        {
        if (!stipple.IsOk() || fillSize.GetHeight() < 4 || fillSize.GetWidth() < 4)
            { return wxNullImage; }
        wxBitmap background(fillSize);
        SetOpacity(background, wxALPHA_TRANSPARENT);
        wxMemoryDC memDC(background);
        memDC.Clear();

        if (direction == Orientation::Horizontal)
            {
            if (!stipple.HasAlpha())
                { stipple.InitAlpha(); }

            const wxSize canvasSize = includeShadow ?
                wxSize(background.GetSize().GetWidth(),
                       background.GetSize().GetHeight()-shadowSize) :
                background.GetSize();

            auto adjustedSize = geometry::calculate_downscaled_size(wxSizeToPair(stipple.GetSize()),
                                                                    wxSizeToPair(canvasSize));
            // if the image is less than the height of the background
            // (but was originally wider than the background),
            // then scale it down to fit the background and let the top of it be cut off.
            if (adjustedSize.second < canvasSize.GetHeight() &&
                stipple.GetHeight() >= canvasSize.GetHeight())
                {
                adjustedSize = std::make_pair(
                    geometry::calculate_rescale_width(
                        wxSizeToPair(stipple.GetSize()), canvasSize.GetHeight()),
                    canvasSize.GetHeight());
                }

            const wxBitmap scaledStipple = stipple.Scale(adjustedSize.first,
                                                         adjustedSize.second, wxIMAGE_QUALITY_HIGH);
            const wxBitmap scaledStippleShadow =
                CreateSilhouette(scaledStipple.ConvertToImage(), false);

            // center, if needed
            const wxCoord yOffset = (adjustedSize.second >= canvasSize.GetHeight()) ?
                0 : (safe_divide<wxCoord>(canvasSize.GetHeight()-adjustedSize.second, 2));

            for (int i = 0; i < canvasSize.GetWidth(); i += scaledStipple.GetWidth()+1)
                {
                if (includeShadow)
                    { memDC.DrawBitmap(scaledStippleShadow, i, yOffset+shadowSize); }
                memDC.DrawBitmap(scaledStipple, i, yOffset);
                }
            }
        else
            {
            if (!stipple.HasAlpha())
                { stipple.InitAlpha(); }

            const wxSize canvasSize = includeShadow ?
                wxSize(background.GetSize().GetWidth()-shadowSize, background.GetSize().GetHeight()) :
                background.GetSize();

            auto adjustedSize = geometry::calculate_downscaled_size(wxSizeToPair(stipple.GetSize()),
                                                                    wxSizeToPair(canvasSize));
            // if the image is less than the width of the background
            // (but was originally wider than the background),
            // then scale it down to fit the background and let the top of it be cut off.
            if (adjustedSize.first < canvasSize.GetWidth() &&
                stipple.GetWidth() >= canvasSize.GetWidth())
                {
                adjustedSize = std::make_pair(canvasSize.GetWidth(),
                    geometry::calculate_rescale_height(
                        wxSizeToPair(stipple.GetSize()), canvasSize.GetWidth()));
                }
            const wxBitmap scaledStipple = stipple.Scale(adjustedSize.first,
                                                         adjustedSize.second, wxIMAGE_QUALITY_HIGH);
            wxBitmap scaledStippleShadow = CreateSilhouette(scaledStipple.ConvertToImage(), false);

            // center image if not as wide as the background
            const wxCoord xOffset = (adjustedSize.first >= canvasSize.GetWidth()) ?
                0 : (safe_divide<wxCoord>(canvasSize.GetWidth()-adjustedSize.first, 2));

            for (int i = canvasSize.GetHeight();
                 i > 0;
                 i -= scaledStipple.GetHeight()+1)
                {
                if (includeShadow)
                    {
                    memDC.DrawBitmap(scaledStippleShadow, xOffset+shadowSize,
                                     i-scaledStipple.GetHeight()+1);
                    }
                memDC.DrawBitmap(scaledStipple, xOffset, i-scaledStipple.GetHeight()+1);
                }
            }

        memDC.SelectObject(wxNullBitmap);

        return background.ConvertToImage();
        }

    //-------------------------------------------
    void Image::SetWidth(const wxCoord width)
        {
        m_size = wxSize(width, geometry::calculate_rescale_height(
            std::make_pair<double, double>(m_originalImg.GetWidth(),
                                           m_originalImg.GetHeight()), width));
        m_frameSize = m_size;
        }

    //-------------------------------------------
    void Image::SetHeight(const wxCoord height)
        {
        m_size = wxSize(geometry::calculate_rescale_width(
            std::make_pair<double, double>(m_originalImg.GetWidth(),
                                           m_originalImg.GetHeight()), height), height);
        m_frameSize = m_size;
        }

    //-------------------------------------------
    void Image::SetSize(const wxSize sz)
        {
        m_size = m_frameSize = sz;
        }

    //-------------------------------------------
    wxSize Image::SetBestSize(const wxSize suggestedSz)
        {
        const auto [width, height] = geometry::calculate_downscaled_size(
            std::make_pair<double, double>(m_originalImg.GetWidth(), m_originalImg.GetHeight()),
            wxSizeToPair(suggestedSz));
        m_size = m_frameSize = wxSize(std::ceil(width), std::ceil(height));
        return m_size;
        }

    //-------------------------------------------
    wxSize Image::ToBestSize(const wxSize originalSz, const wxSize suggestedSz)
        {
        // if original is smaller, then upscale
        if (originalSz.GetWidth() <= suggestedSz.GetWidth() &&
            originalSz.GetHeight() <= suggestedSz.GetHeight())
            {
            const auto [width, height] = geometry::calculate_upscaled_size(
                std::make_pair<double, double>(originalSz.GetWidth(), originalSz.GetHeight()),
                wxSizeToPair(suggestedSz));
            return wxSize(std::ceil(width), std::ceil(height));
            }
        // if larger, then downscale
        else if (originalSz.GetWidth() >= suggestedSz.GetWidth() &&
            originalSz.GetHeight() >= suggestedSz.GetHeight())
            {
            const auto [width, height] = geometry::calculate_downscaled_size(
                std::make_pair<double, double>(originalSz.GetWidth(), originalSz.GetHeight()),
                wxSizeToPair(suggestedSz));
            return wxSize(std::ceil(width), std::ceil(height));
            }
        else
            { return originalSz; }
        }

    //-------------------------------------------
    void Image::SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                               [[maybe_unused]] const double parentScaling)
        {
        wxASSERT_LEVEL_2_MSG(!IsFreeFloating(),
                             L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            { return; }
        if (GetAnchoring() == Anchoring::Center)
            {
            SetAnchorPoint(wxPoint(rect.GetLeft()+(rect.GetWidth()/2),
                           rect.GetTop()+(rect.GetHeight()/2)));
            }
        else if (GetAnchoring() == Anchoring::TopLeftCorner)
            { SetAnchorPoint(rect.GetTopLeft()); }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            { SetAnchorPoint(rect.GetTopRight()); }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            { SetAnchorPoint(rect.GetBottomLeft()); }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            { SetAnchorPoint(rect.GetBottomRight()); }
        // adjust the height to fit the bounding box
        if (GetResizeMethod() == ResizeMethod::DownscaleOrUpscale)
            {
            m_size = wxSize(geometry::calculate_rescale_width(
                std::make_pair<double, double>(m_originalImg.GetSize().GetWidth(),
                                               m_originalImg.GetSize().GetHeight()),
                rect.GetHeight()), rect.GetHeight());
            // height adjusted to the rect, but if it is too wide now then we need to
            // adjust the width to the rect and rescale the height to this new width
            if (m_size.GetWidth() > rect.GetWidth())
                {
                m_size = wxSize(rect.GetWidth(), geometry::calculate_rescale_height(
                    std::make_pair<double, double>(m_size.GetWidth(),m_size.GetHeight()),
                                                   rect.GetWidth()));
                }
            }
        else if (GetResizeMethod() == ResizeMethod::DownscaleOnly)
            {
            auto downSize = geometry::calculate_downscaled_size(
                    std::make_pair<double, double>(m_originalImg.GetSize().GetWidth(), m_originalImg.GetHeight()),
                    std::make_pair<double, double>(rect.GetWidth(), rect.GetHeight()));
            m_size = wxSize(downSize.first, downSize.second);
            }
        else if (GetResizeMethod() == ResizeMethod::UpscaleOnly)
            {
            auto downSize = geometry::calculate_upscaled_size(
                    std::make_pair<double, double>(m_originalImg.GetSize().GetWidth(), m_originalImg.GetHeight()),
                    std::make_pair<double, double>(rect.GetWidth(), rect.GetHeight()));
            m_size = wxSize(downSize.first, downSize.second);
            }
        m_size *= safe_divide<double>(1.0f, GetScaling());
        m_frameSize = rect.GetSize()*safe_divide<double>(1.0f, GetScaling());
        }

    //-------------------------------------------
    wxRect Image::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        const wxCoord width(m_frameSize.GetWidth()*GetScaling());
        const wxCoord height(m_frameSize.GetHeight()*GetScaling());
        wxRect boundingBox;

        if (GetAnchoring() == Anchoring::Center)
            {
            boundingBox = wxRect(GetAnchorPoint()-wxPoint(width/2,height/2),
                                 GetAnchorPoint()+wxPoint(width/2,height/2));
            }
        else if (GetAnchoring() == Anchoring::TopLeftCorner)
            { boundingBox = wxRect(GetAnchorPoint(), wxSize(width,height)); }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            { boundingBox = wxRect(GetAnchorPoint()-wxSize(width,0), wxSize(width,height)); }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            { boundingBox = wxRect(GetAnchorPoint()-wxPoint(0,height), wxSize(width,height)); }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            { boundingBox = wxRect(GetAnchorPoint()-wxSize(width,height), wxSize(width,height)); }
        if (IsFreeFloating())
            {
            boundingBox.Offset((boundingBox.GetLeftTop()*GetScaling())-boundingBox.GetLeftTop());
            }
        return boundingBox;
        }

    //-------------------------------------------
    wxImage Image::LoadFile(const wxString& filePath)
        {
        // if SVG, load it as such (using the embedded size and aspect ratio)
        if (const wxFileName fn(filePath); fn.GetExt().MakeLower().CmpNoCase(L"svg") == 0)
            {
            const wxSize svgSize = GetSVGSize(filePath);

            return wxBitmapBundle::FromSVGFile(filePath, svgSize).
                GetBitmap(svgSize).ConvertToImage();
            }

        // otherwise, load as a raster image file
        try
            {
            MemoryMappedFile mappedImg(filePath, true, true);
            if (!mappedImg.IsOk())
                { return wxNullImage; }

            wxMemoryInputStream stream(static_cast<const char*>(mappedImg.GetStream()),
                                                                mappedImg.GetMapSize());
            wxImage image(stream);
            // parse EXIF
            if (image.IsOk() && image.GetType() == wxBITMAP_TYPE_JPEG)
                {
                easyexif::EXIFInfo result;
                if (result.parseFrom(static_cast<const unsigned char*>(mappedImg.GetStream()),
                                                                       mappedImg.GetMapSize()) == 0)
                    {
                    // correct the orientation (if necessary)
                    switch (result.Orientation)
                        {
                    // image data starts at lower right of image, flip it
                    case 3:
                        image = image.Rotate180();
                        break;
                    // image data starts at upper right of image, turn it
                    case 6:
                        image = image.Rotate90(true);
                        break;
                    // image data starts at lower left of image, turn it
                    case 8:
                        image = image.Rotate90(false);
                        }
                    }
                }

            return image;
            }
        catch (...)
            { return wxNullImage; }
        }

    //-------------------------------------------
    wxBitmapType Image::GetImageFileTypeFromExtension(wxString& ext)
        {
        ext = (ext.find(L'.') == std::wstring::npos) ? ext : wxFileName(ext).GetExt();
        wxBitmapType imageType = wxBITMAP_TYPE_ANY;
        if (ext.CmpNoCase(L"jpg") == 0 ||
            ext.CmpNoCase(L"jpeg") == 0 ||
            ext.CmpNoCase(L"jpe") == 0)
            { imageType = wxBITMAP_TYPE_JPEG; }
        else if (ext.CmpNoCase(L"gif") == 0)
            { imageType = wxBITMAP_TYPE_GIF; }
        else if (ext.CmpNoCase(L"png") == 0)
            { imageType = wxBITMAP_TYPE_PNG; }
        else if (ext.CmpNoCase(L"bmp") == 0)
            { imageType = wxBITMAP_TYPE_BMP; }
        else if (ext.CmpNoCase(L"tif") == 0 ||
                 ext.CmpNoCase(L"tiff") == 0)
            { imageType = wxBITMAP_TYPE_TIF; }
        else if (ext.CmpNoCase(L"pcx") == 0)
            { imageType = wxBITMAP_TYPE_PCX; }
        else if (ext.CmpNoCase(L"tga") == 0)
            { imageType = wxBITMAP_TYPE_TGA; }
        // no enum value for this, but need to set it to something
        else if (ext.CmpNoCase(L"svg") == 0)
            { imageType = wxBITMAP_TYPE_ANY; }
        else
            { imageType = wxBITMAP_TYPE_PNG; }
        return imageType;
        }

    //-------------------------------------------
    wxRect Image::Draw(wxDC& dc) const
        {
        if (!IsShown() || !IsOk() || !m_img.IsOk())
            { return wxRect(); }
        if (IsInDragState())
            { return GetBoundingBox(dc); }

        if (GetClippingRect())
            { dc.SetClippingRegion(GetClippingRect().value()); }

        // if the size or scaling has changed, then rescale from
        // the original image to maintain fidelity
        const wxSize scaledSize(GetImageSize().GetWidth()*GetScaling(),
                                GetImageSize().GetHeight()*GetScaling());
        if (m_img.GetSize() != scaledSize)
            {
            m_img = m_originalImg;
            m_img.Rescale(scaledSize.GetWidth(), scaledSize.GetHeight(),
                          wxIMAGE_QUALITY_HIGH);
            }

        SetOpacity(m_img, m_opacity, true);

        // Draw the shadow. This needs to be a polygon outside of the image
        // in case the image is translucent.
        if (GetShadowType() != ShadowType::NoShadow && !IsSelected() &&
            GetBoundingBox(dc).GetHeight() > ScaleToScreenAndCanvas(GetShadowOffset()))
            {
            wxDCPenChanger pc(dc, wxPen(GetShadowColour(), ScaleToScreenAndCanvas(1)));
            wxDCBrushChanger bc(dc, wxBrush(GetShadowColour()));
            const wxCoord scaledShadowOffset = ScaleToScreenAndCanvas(GetShadowOffset());
            if (GetShadowType() == ShadowType::RightSideAndBottomShadow)
                {
                wxPoint shadowPts[7];
                shadowPts[0] = GetBoundingBox(dc).GetLeftBottom() +
                               wxPoint(scaledShadowOffset,0);
                shadowPts[1] = GetBoundingBox(dc).GetLeftBottom() +
                               wxPoint(scaledShadowOffset,scaledShadowOffset);
                shadowPts[2] = GetBoundingBox(dc).GetRightBottom() +
                               wxPoint(scaledShadowOffset,scaledShadowOffset);
                shadowPts[3] = GetBoundingBox(dc).GetRightTop() +
                               wxPoint(scaledShadowOffset,scaledShadowOffset);
                shadowPts[4] = GetBoundingBox(dc).GetRightTop() +
                               wxPoint(0,scaledShadowOffset);
                shadowPts[5] = GetBoundingBox(dc).GetRightBottom();
                shadowPts[6] = shadowPts[0];//close polygon
                dc.DrawPolygon(std::size(shadowPts), shadowPts);
                }
            else if (GetShadowType() == ShadowType::RightSideShadow)
                {
                wxPoint shadowPts[4];
                shadowPts[0] = GetBoundingBox(dc).GetRightBottom() +
                               wxPoint(scaledShadowOffset,0);
                shadowPts[1] = GetBoundingBox(dc).GetRightTop() +
                               wxPoint(scaledShadowOffset,scaledShadowOffset);
                shadowPts[2] = GetBoundingBox(dc).GetRightTop() +
                               wxPoint(0,scaledShadowOffset);
                shadowPts[3] = GetBoundingBox(dc).GetRightBottom();
                dc.DrawPolygon(std::size(shadowPts), shadowPts);
                }
            }

        // position the image inside of its (possibly) larger box
        wxPoint imgTopLeftCorner(GetBoundingBox(dc).GetLeftTop());
        // horizontal page alignment
        if ((GetFrameSize() == GetImageSize()) ||
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned))
            { /*noop*/ }
        else if (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered)
            {
            imgTopLeftCorner.x += safe_divide<double>(GetBoundingBox(dc).GetWidth(), 2) -
                safe_divide<double>(GetImageSize().GetWidth() * GetScaling(), 2);
            }
        else if (GetPageHorizontalAlignment() == PageHorizontalAlignment::RightAligned)
            {
            imgTopLeftCorner.x += GetBoundingBox(dc).GetWidth() -
               GetImageSize().GetWidth() * GetScaling();
            }
        // vertical page aligment
        if ((GetFrameSize() == GetImageSize()) ||
            (GetPageVerticalAlignment() == PageVerticalAlignment::TopAligned))
            { /*noop*/ }
        else if (GetPageVerticalAlignment() == PageVerticalAlignment::Centered)
            {
            imgTopLeftCorner.y += safe_divide<double>(GetBoundingBox(dc).GetHeight(), 2) -
                safe_divide<double>(GetImageSize().GetHeight() * GetScaling(), 2);
            }
        else if (GetPageVerticalAlignment() == PageVerticalAlignment::BottomAligned)
            {
            imgTopLeftCorner.y += GetBoundingBox(dc).GetHeight() -
               (GetImageSize().GetHeight() * GetScaling());
            }

        dc.DrawBitmap(wxBitmap(m_img), imgTopLeftCorner, true);

        // draw the outline
        wxPoint pts[5];
        GraphItems::Polygon::GetRectPoints(GetBoundingBox(dc), pts);
        pts[4] = pts[0]; // close the square
        if (GetPen().IsOk())
            {
            wxPen scaledPen(GetPen());
            scaledPen.SetWidth(ScaleToScreenAndCanvas(GetPen().GetWidth()));
            wxDCPenChanger pc(dc, IsSelected() ?
                wxPen(*wxBLACK, 2*scaledPen.GetWidth(), wxPENSTYLE_DOT) : scaledPen);
            dc.DrawLines(std::size(pts), pts);
            }
        // just draw selection outline if regular pen isn't in use
        else if (IsSelected())
            {
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, 2, wxPENSTYLE_DOT));
            dc.DrawLines(std::size(pts), pts);
            }

        if (GetClippingRect())
            { dc.DestroyClippingRegion(); }
        return GetBoundingBox(dc);
        }
    }
