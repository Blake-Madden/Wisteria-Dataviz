///////////////////////////////////////////////////////////////////////////////
// Name:        image.cpp
// Author:      Blake Madden (portions from Bhumika Thatte, Raghavendra Sri, Prasad R V, and Avijnata)
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause, CPOL-1.02
///////////////////////////////////////////////////////////////////////////////

#include "image.h"
#include "polygon.h"
#include <array>

using namespace Wisteria::Colors;

namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------
    wxSize Image::GetSVGSize(const wxString& filePath)
        {
        wxXmlDocument doc;
        if (!doc.Load(filePath))
            { return wxSize(32, 32); }
        wxXmlNode* docNode = doc.GetDocumentNode()->GetChildren();
        if (docNode == nullptr)
            { return wxSize(32, 32); }
        wxString heightStr, widthStr, viewBoxStr;
        wxSize sz{ wxDefaultCoord, wxDefaultCoord };
        if (docNode->GetAttribute(L"width", &widthStr))
            { widthStr.ToInt(&sz.x); }
        if (docNode->GetAttribute(L"height", &heightStr))
            { heightStr.ToInt(&sz.y); }
        // if no width or height attributes
        if (!sz.IsFullySpecified())
            {
            if (docNode->GetAttribute(L"viewBox", &viewBoxStr))
                {
                const wxRegEx re(L"([[:digit:].]+[ ]+){2}([[:digit:].]+)[ ]+([[:digit:].]+)");
                if (re.Matches(viewBoxStr) && re.GetMatchCount() >= 3)
                    {
                    widthStr = re.GetMatch(viewBoxStr, 2);
                    heightStr = re.GetMatch(viewBoxStr, 3);
                    widthStr.ToInt(&sz.x);
                    heightStr.ToInt(&sz.y);
                    }
                else
                    { return wxSize(32, 32); }
                }
            else
                { return wxSize(32, 32); }
            }

        return sz;
        }

    //----------------------------------------------------------
    wxImage Image::ShrinkImageToRect(const wxImage& img, const wxRect rect)
        {
        if (rect.GetWidth() >= img.GetWidth() &&
            rect.GetHeight() >= img.GetHeight())
            { return img; }

        const auto [width, height] = geometry::downscaled_size(
            std::make_pair<double, double>(img.GetWidth(), img.GetHeight()),
            wxSizeToPair(rect.GetSize()));
        return img.Scale(std::ceil(width), std::ceil(height), wxIMAGE_QUALITY_HIGH);
        }

    //----------------------------------------------------------
    wxImage Image::CropImageToRect(const wxImage& img, const wxRect rect, const bool centerImage)
        {
        wxImage croppedImg;
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
                const auto scaledHeight = geometry::rescaled_height(
                    std::make_pair(img.GetWidth(),
                                   img.GetHeight()),
                    rect.GetSize().GetWidth());
                croppedImg = img.Scale(rect.GetSize().GetWidth(), scaledHeight,
                    wxImageResizeQuality::wxIMAGE_QUALITY_HIGH);

                const auto crop = croppedImg.GetHeight() -
                    rect.GetSize().GetHeight();
                croppedImg = croppedImg.GetSubImage(
                    wxRect(wxPoint(0, centerImage ? std::floor(safe_divide<double>(crop, 2)) : 0),
                           rect.GetSize()));
                wxASSERT_LEVEL_2_MSG((croppedImg.GetSize().GetHeight() >=
                    rect.GetSize().GetHeight()),
                    wxString::Format(L"Common image not scaled height-wise large enough! %d vs %d",
                        croppedImg.GetSize().GetHeight(),
                        rect.GetSize().GetHeight()));
                }
            else
                {
                const auto scaledWidth = geometry::rescaled_width(
                    std::make_pair(img.GetWidth(),
                                   img.GetHeight()),
                    rect.GetSize().GetHeight());
                croppedImg = img.Scale(scaledWidth, rect.GetSize().GetHeight(),
                    wxImageResizeQuality::wxIMAGE_QUALITY_HIGH);

                const auto crop = croppedImg.GetWidth() -
                                  rect.GetSize().GetWidth();
                croppedImg = croppedImg.GetSubImage(
                    wxRect(wxPoint(centerImage ? std::floor(safe_divide<double>(crop, 2)) : 0, 0),
                           rect.GetSize()));
                wxASSERT_LEVEL_2_MSG((croppedImg.GetSize().GetWidth() >=
                              rect.GetSize().GetWidth()),
                             wxString::Format(L"Common image not scaled width-wise large enough! %d vs %d",
                                              croppedImg.GetSize().GetWidth(),
                                              rect.GetSize().GetWidth()));
                }
            }
        return croppedImg;
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
                          rgbData[(i*3) + 1] == greenChannel &&
                          rgbData[(i*3) + 2] == blueChannel))
                        { alphaData[i] = opacity; } // cppcheck-suppress unreadVariable
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
    wxImage Image::OilPainting(const wxImage& image, const uint8_t radius /*= 2*/,
                               const float intensity /*= 20*/)
        {
        if (!image.IsOk())
            { return wxNullImage; }

        wxImage outImg{ image.Copy()};
        const auto imgInData = image.GetData();
        const auto imgOutData = outImg.GetData();

        // Border pixels (depends on radius) will become black.
        // On increasing radius boundary pixels should set as black.
        std::memset(imgOutData, 0, static_cast<size_t>(image.GetWidth() * image.GetHeight() * 3));

        // If total bytes in a row of image is not divisible by four, 
        // blank bytes will be padded to the end of the row.
        // nBytesInARow bytes are the actual size of a row instead of nWidth * 3.
        // If width is 9, then actual bytes in a row will be 28, and not 27.
        const int nBytesInARow = std::ceil(image.GetWidth() * 3 / 4.0) * 4.0;

        constexpr auto rgbBufferSize{ 256 };

        // Note that radius pixels are avoided from left, right, top, and bottom edges.
        // Go to the next row of pixels...
        #pragma omp parallel for
        for (int nY = radius; nY < image.GetHeight() - radius; ++nY)
            {
            // ...and go across, pixel-by-pixel
            for (int nX = radius; nX < image.GetWidth() - radius; ++nX)
                {
                // Reset calculations of last pixel.
                std::array<int, rgbBufferSize> nIntensityCount{ 0 };
                std::array<int, rgbBufferSize> nSumR{ 0 };
                std::array<int, rgbBufferSize> nSumG{ 0 };
                std::array<int, rgbBufferSize> nSumB{ 0 };

                // Find intensities of nearest radius pixels in four direction.
                for (int nY_O = -radius; nY_O <= radius; ++nY_O)
                    {
                    for (int nX_O = -radius; nX_O <= radius; ++nX_O)
                        {
                        const int nR = imgInData[(nX + nX_O) * 3 + (nY + nY_O) * nBytesInARow];
                        const int nG = imgInData[(nX + nX_O) * 3 + (nY + nY_O) * nBytesInARow + 1];
                        const int nB = imgInData[(nX + nX_O) * 3 + (nY + nY_O) * nBytesInARow + 2];

                        // Find intensity of RGB value and apply intensity level.
                        const int nCurIntensity =
                            std::clamp<int>((((nR + nG + nB) / 3.0) * intensity) / 255,
                                            0, (rgbBufferSize-1));
                        ++nIntensityCount[nCurIntensity];

                        nSumR[nCurIntensity] += nR;
                        nSumG[nCurIntensity] += nG;
                        nSumB[nCurIntensity] += nB;
                        }
                    }

                int nCurMax{ 0 };
                int nMaxIndex{ 0 };
                for (int nI = 0; nI < rgbBufferSize; ++nI)
                    {
                    if (nIntensityCount[nI] > nCurMax)
                        {
                        nCurMax = nIntensityCount[nI];
                        nMaxIndex = nI;
                        }
                    }

                assert(nMaxIndex >= 0 && nMaxIndex < rgbBufferSize &&
                    L"Invalid buffer index in oil painting effect!");
                assert(((nX) * 3 + (nY)*nBytesInARow + 2) <
                    (image.GetWidth() * image.GetHeight() * 3) &&
                    L"Invalid image data index in oil painting effect!");

                imgOutData[(nX) * 3 + (nY)*nBytesInARow] = nSumR[nMaxIndex] / nCurMax;
                imgOutData[(nX) * 3 + (nY)*nBytesInARow + 1] = nSumG[nMaxIndex] / nCurMax;
                imgOutData[(nX) * 3 + (nY)*nBytesInARow + 2] = nSumB[nMaxIndex] / nCurMax;
                }
            }

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::ApplyEffect(const Wisteria::ImageEffect effect, const wxImage& img)
        {
        if (effect == Wisteria::ImageEffect::Grayscale)
            { return img.ConvertToGreyscale(); }
        else if (effect == Wisteria::ImageEffect::BlurHorizontal)
            { return img.BlurHorizontal(10); }
        else if (effect == Wisteria::ImageEffect::BlurVertical)
            { return img.BlurVertical(10); }
        else if (effect == Wisteria::ImageEffect::Sepia)
            { return Wisteria::GraphItems::Image::Sepia(img); }
        else if (effect == Wisteria::ImageEffect::FrostedGlass)
            { return Wisteria::GraphItems::Image::FrostedGlass(img); }
        else if (effect == Wisteria::ImageEffect::OilPainting)
            { return Wisteria::GraphItems::Image::OilPainting(img); }
        else
            { return img; }
        }

    //-------------------------------------------
    wxImage Image::FrostedGlass(const wxImage& image,
        const Wisteria::Orientation orientation /*= Orientation::Both*/,
        const uint8_t coarseness /*= 50*/)
        {
        if (!image.IsOk())
            { return wxNullImage; }

        wxImage outImg{ image.Copy()};
        const auto imgInData = image.GetData();
        const auto imgOutData = outImg.GetData();
        const auto byteCount{ image.GetWidth() * image.GetHeight() * 3 };

        std::random_device dev;
        std::mt19937 twister(dev());
        std::uniform_real_distribution<> distro(0, 1);

        const auto findXInBound = [&image](const int x)
            {
            const int x1 = (x < 0) ?
                0 :
                (x >= image.GetWidth() * 3) ?
                (image.GetWidth() * 3) - 1 :
                x;

            const auto stepBackToRedChannel = x1 % 3;
            return x1 - stepBackToRedChannel;
            };

        const auto findYInBound = [&image](const int y)
            {
            return (y < 0) ?
                0 :
                (y >= image.GetHeight()) ?
                image.GetHeight() - 1 :
                y;
            };

        // horizontal and bidirectional
        if (orientation == Orientation::Horizontal || orientation == Orientation::Both)
            {
            for (auto rowCounter = 0; rowCounter < image.GetHeight(); ++rowCounter)
                {
                auto w2 = image.GetWidth() * 3 * rowCounter;
                int y{ 0 };

                // horizontally oriented glass
                if (orientation == Orientation::Horizontal)
                    {
                    y = static_cast<int>(rowCounter + (distro(twister) - 0.5) * coarseness);
                    y = findYInBound(y);
                    }

                for (auto columCounter = 0; columCounter < image.GetWidth() * 3; columCounter += 3)
                    {
                    int x = static_cast<int>(columCounter + (distro(twister) - 0.5) * coarseness);

                    // generally oriented glass
                    if (orientation == Orientation::Both)
                        {
                        y = static_cast<int>(rowCounter + (distro(twister) - 0.5) * coarseness);
                        y = findYInBound(y);
                        }

                    x = findXInBound(x);

                    // source pixel
                    auto w1 = image.GetWidth() * 3 * y + x;
                    assert(w1 + 2 < byteCount && L"Invalid index in image buffer!");
                    const auto r = imgInData[w1];
                    const auto g = imgInData[w1 + 1];
                    const auto b = imgInData[w1 + 2];

                    // target pixel
                    w1 = w2 + columCounter;
                    assert(w1 + 2 < byteCount && L"Invalid index in image buffer!");
                    imgOutData[w1] = r;
                    imgOutData[w1 + 1] = g;
                    imgOutData[w1 + 2] = b;
                    }
                }
            }
        else // Vertical
            {
            for (auto columnCounter = 0; columnCounter < image.GetWidth() * 3; columnCounter += 3)
                {
                const auto x =
                    findXInBound(
                        static_cast<int>(columnCounter + (distro(twister) - 0.5) * coarseness));
                for (auto rowCounter = 0; rowCounter < image.GetHeight(); ++rowCounter)
                    {
                    const auto y =
                        findYInBound(
                            static_cast<int>(rowCounter + (distro(twister) - 0.5) * coarseness));

                    // Source pixel
                    auto w1 = image.GetWidth() * 3 * y + x;
                    assert(w1 + 2 < byteCount && L"Invalid index in image buffer!");
                    const auto r = imgInData[w1];
                    const auto g = imgInData[w1 + 1];
                    const auto b = imgInData[w1 + 2];

                    // Target pixel
                    w1 = image.GetWidth() * 3 * rowCounter + columnCounter;
                    assert(w1 + 2 < byteCount && L"Invalid index in image buffer!");
                    imgOutData[w1] = r;
                    imgOutData[w1 + 1] = g;
                    imgOutData[w1 + 2] = b;
                    }
                }
            }

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::Sepia(const wxImage& image, const uint8_t magnitude /*= 50*/)
        {
        if (!image.IsOk())
            { return wxNullImage; }

        wxImage outImg{ image.Copy()};
        const auto imgInData = image.GetData();
        const auto imgOutData = outImg.GetData();

        const auto byteCount = image.GetWidth() * image.GetHeight() * 3;
        const double threshold = magnitude * 255.0 / 100.0;
        const double thres6By7 = 7.0 * threshold / 6.0;
        const double thres6 = threshold / 6.0;
        const double thres7 = threshold / 7.0;

        // Target image
        for (auto index = 0; index < byteCount; index += 3)
            {
            const auto r = imgInData[index];
            const auto g = imgInData[index + 1];
            const auto b = imgInData[index + 2];
            // Grayscale
            const auto intensity = 0.3 * r + 0.6 * g + 0.1 * b;

            // Red
            auto tone = (intensity > threshold) ?
                255.0 :
                intensity + 255.0 - threshold;
            const auto dRed = tone;

            // Green
            tone = (intensity > thres6By7) ?
                255.0 :
                intensity + 255.0 - thres6By7;
            auto dGreen = tone;

            // Blue
            tone = (intensity < thres6) ?
                0 :
                intensity - thres6;
            auto dBlue = tone;

            tone = thres7;
            if (dGreen < tone)
                dGreen = tone;
            if (dBlue < tone)
                dBlue = tone;

            imgOutData[index] = dRed;
            imgOutData[index + 1] = dGreen;
            imgOutData[index + 2] = dBlue;
            }

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::ChangeColor(const wxImage& image, const wxColour srcColor,
                               const wxColour destColor)
        {
        if (!image.IsOk())
            { return wxNullImage; }

        wxImage img{ image.Copy()};
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
        wxImage Silhouette = image.ConvertToMono(0, 0, 0);
        SetColorTransparent(Silhouette, *wxWHITE);
        if (!opaque)
            {
            Silhouette = ChangeColor(Silhouette, *wxBLACK,
                                     ColorBrewer::GetColor(Color::LightGray));
            }
        return Silhouette;
        }

    //-------------------------------------------
    wxImage Image::CreateColorFilteredImage(const wxImage& image,
                                            const wxColour color,
                                            const uint8_t opacity /*= 100*/)
        {
        wxBitmap bmp(image);
        wxMemoryDC memDC(bmp);
        auto gc = wxGraphicsContext::Create(memDC);
        assert(gc && L"Failed to get graphics context for filtered image!");
        if (gc)
            {
            gc->SetBrush(wxBrush(Colors::ColorContrast::ChangeOpacity(color, opacity)));
            gc->DrawRectangle(0, 0, memDC.GetSize().GetWidth(), memDC.GetSize().GetHeight());
            wxDELETE(gc);
            }
        memDC.SelectObject(wxNullBitmap);
        return bmp.ConvertToImage();
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
                                        fillSize.GetWidth() : fillSize.GetWidth() * math_constants::quarter,
                                    (direction == Orientation::Vertical) ?
                                        fillSize.GetHeight() * math_constants::quarter : fillSize.GetHeight()),
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
        assert(bmp.IsOk());
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
        assert(bmp.IsOk());
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

            const auto adjustedHeight = std::min(canvasSize.GetHeight(), stipple.GetHeight());
            auto adjustedWidth = geometry::rescaled_width(wxSizeToPair(stipple.GetSize()),
                                                          adjustedHeight);

            const wxBitmap scaledStipple = stipple.Scale(adjustedWidth,
                                                         adjustedHeight, wxIMAGE_QUALITY_HIGH);
            const wxBitmap scaledStippleShadow =
                CreateSilhouette(scaledStipple.ConvertToImage(), false);

            // center, if needed
            const wxCoord yOffset = (adjustedHeight >= canvasSize.GetHeight()) ?
                0 : (safe_divide<wxCoord>(canvasSize.GetHeight() - adjustedHeight, 2));

            for (int i = 0; i < canvasSize.GetWidth(); i += scaledStipple.GetWidth()+1)
                {
                if (includeShadow)
                    { memDC.DrawBitmap(scaledStippleShadow, i, yOffset + shadowSize); }
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

            const auto adjustedWidth = std::min(canvasSize.GetWidth(), stipple.GetWidth());
            auto adjustedHeight = geometry::rescaled_height(wxSizeToPair(stipple.GetSize()),
                                                            adjustedWidth);
            const wxBitmap scaledStipple = stipple.Scale(adjustedWidth,
                                                         adjustedHeight, wxIMAGE_QUALITY_HIGH);
            wxBitmap scaledStippleShadow = CreateSilhouette(scaledStipple.ConvertToImage(), false);

            // center image if not as wide as the background
            const wxCoord xOffset = (adjustedWidth >= canvasSize.GetWidth()) ?
                0 : (safe_divide<wxCoord>(canvasSize.GetWidth() - adjustedWidth, 2));

            for (int i = canvasSize.GetHeight();
                 i > 0;
                 i -= scaledStipple.GetHeight()+1)
                {
                if (includeShadow)
                    {
                    memDC.DrawBitmap(scaledStippleShadow, xOffset+shadowSize,
                                     i - scaledStipple.GetHeight() + 1);
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
        m_size = wxSize(width, geometry::rescaled_height(
            std::make_pair<double, double>(m_originalImg.GetWidth(),
                                           m_originalImg.GetHeight()), width));
        m_frameSize = m_size;
        }

    //-------------------------------------------
    void Image::SetHeight(const wxCoord height)
        {
        m_size = wxSize(geometry::rescaled_width(
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
        m_size = m_frameSize = GetBestSize(suggestedSz);
        return m_size;
        }

    //-------------------------------------------
    wxSize Image::GetBestSize(const wxSize suggestedSz) const
        {
        const auto [width, height] = geometry::downscaled_size(
            std::make_pair<double, double>(m_originalImg.GetWidth(), m_originalImg.GetHeight()),
            wxSizeToPair(suggestedSz));
        return wxSize(std::ceil(width), std::ceil(height));
        }

    //-------------------------------------------
    wxSize Image::ToBestSize(const wxSize originalSz, const wxSize suggestedSz)
        {
        // if original is smaller, then upscale
        if (originalSz.GetWidth() <= suggestedSz.GetWidth() &&
            originalSz.GetHeight() <= suggestedSz.GetHeight())
            {
            const auto [width, height] = geometry::upscaled_size(
                std::make_pair<double, double>(originalSz.GetWidth(), originalSz.GetHeight()),
                wxSizeToPair(suggestedSz));
            return wxSize(std::ceil(width), std::ceil(height));
            }
        // if larger, then downscale
        else if (originalSz.GetWidth() >= suggestedSz.GetWidth() &&
            originalSz.GetHeight() >= suggestedSz.GetHeight())
            {
            const auto [width, height] = geometry::downscaled_size(
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
        assert(!IsFreeFloating() &&
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
            m_size = wxSize(geometry::rescaled_width(
                std::make_pair<double, double>(m_originalImg.GetSize().GetWidth(),
                                               m_originalImg.GetSize().GetHeight()),
                rect.GetHeight()), rect.GetHeight());
            // height adjusted to the rect, but if it is too wide now then we need to
            // adjust the width to the rect and rescale the height to this new width
            if (m_size.GetWidth() > rect.GetWidth())
                {
                m_size = wxSize(rect.GetWidth(), geometry::rescaled_height(
                    std::make_pair<double, double>(m_size.GetWidth(),m_size.GetHeight()),
                                                   rect.GetWidth()));
                }
            }
        else if (GetResizeMethod() == ResizeMethod::DownscaleOnly)
            {
            auto downSize = geometry::downscaled_size(
                    std::make_pair<double, double>(m_originalImg.GetSize().GetWidth(), m_originalImg.GetHeight()),
                    std::make_pair<double, double>(rect.GetWidth(), rect.GetHeight()));
            m_size = wxSize(downSize.first, downSize.second);
            }
        else if (GetResizeMethod() == ResizeMethod::UpscaleOnly)
            {
            auto downSize = geometry::upscaled_size(
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
        // weird exception that auto-buffering won't help, so explain it to the user
        catch (const MemoryMappedFileCloudFileError&)
            {
            wxLogWarning(
                wxString::Format(_(L"%s: unable to open file from Cloud service."),
                    filePath),
                _(L"Error"), wxOK|wxICON_EXCLAMATION);
            return wxNullImage;
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
        m_img = m_originalImg;
        if (m_img.GetSize() != scaledSize)
            {
            m_img.Rescale(scaledSize.GetWidth(), scaledSize.GetHeight(),
                          wxIMAGE_QUALITY_HIGH);
            }

        SetOpacity(m_img, m_opacity, true);

        // Draw the shadow. This needs to be a polygon outside of the image
        // in case the image is translucent.
        if (GetShadowType() != ShadowType::NoDisplay && !IsSelected() &&
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
                shadowPts[6] = shadowPts[0]; // close polygon
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
        // vertical page alignment
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
