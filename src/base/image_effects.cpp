///////////////////////////////////////////////////////////////////////////////
// Name:        image_effects.cpp
// Author:      Blake Madden (portions from Bhumika Thatte, Raghavendra Sri,
//                            Prasad R V, and Avijnata)
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause, CPOL-1.02
///////////////////////////////////////////////////////////////////////////////

#include "image.h"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    Image::BorderEdges Image::FindBorderEdges(const wxImage& img, const uint8_t colorTolerance,
                                              const wxColour& baseColor)
        {
        BorderEdges edges;

        const int width{ img.GetWidth() };
        const int height{ img.GetHeight() };
        const unsigned char* rgbData{ img.GetData() };
        const unsigned char* alphaData{ img.HasAlpha() ? img.GetAlpha() : nullptr };

        if (width <= 0 || height <= 0 || rgbData == nullptr)
            {
            return edges;
            }

        const uint8_t tolerance{ colorTolerance };
        const unsigned char baseRed{ baseColor.Red() };
        const unsigned char baseGreen{ baseColor.Green() };
        const unsigned char baseBlue{ baseColor.Blue() };

        // true if the pixel at (x, y) is close enough to baseColor (or transparent)
        // to be considered part of the border
        const auto isBorderPixel = [&](const int x, const int y) noexcept
        {
            const size_t pixelIndex{ (static_cast<size_t>(y) * width) + x };
            if (alphaData != nullptr && alphaData[pixelIndex] == wxALPHA_TRANSPARENT)
                {
                return true;
                }
            const unsigned char red{ rgbData[pixelIndex * 3] };
            const unsigned char green{ rgbData[(pixelIndex * 3) + 1] };
            const unsigned char blue{ rgbData[(pixelIndex * 3) + 2] };
            return (std::abs(red - baseRed) <= tolerance) &&
                   (std::abs(green - baseGreen) <= tolerance) &&
                   (std::abs(blue - baseBlue) <= tolerance);
        };

        edges.m_rowLeftEdges.reserve(height);
        edges.m_rowRightEdges.reserve(height);
        edges.m_colTopEdges.reserve(width);
        edges.m_colBottomEdges.reserve(width);

        for (int y = 0; y < height; ++y)
            {
            int left{ -1 };
            for (int x = 0; x < width; ++x)
                {
                if (!isBorderPixel(x, y))
                    {
                    left = x;
                    break;
                    }
                }
            if (left == -1)
                {
                // entire row is border
                continue;
                }
            int right{ left };
            for (int x = width - 1; x > right; --x)
                {
                if (!isBorderPixel(x, y))
                    {
                    right = x;
                    break;
                    }
                }
            edges.m_rowLeftEdges.emplace_back(y, left);
            edges.m_rowRightEdges.emplace_back(y, right);
            }

        for (int x = 0; x < width; ++x)
            {
            int top{ -1 };
            for (int y = 0; y < height; ++y)
                {
                if (!isBorderPixel(x, y))
                    {
                    top = y;
                    break;
                    }
                }
            if (top == -1)
                {
                // entire column is border
                continue;
                }
            int bottom{ top };
            for (int y = height - 1; y > bottom; --y)
                {
                if (!isBorderPixel(x, y))
                    {
                    bottom = y;
                    break;
                    }
                }
            edges.m_colTopEdges.emplace_back(x, top);
            edges.m_colBottomEdges.emplace_back(x, bottom);
            }

        return edges;
        }

    //-------------------------------------------
    wxImage Image::CropImageBorder(const wxImage& img, const uint8_t colorTolerance,
                                   const wxColour& baseColor)
        {
        if (!img.IsOk())
            {
            return img;
            }

        BorderEdges edges{ FindBorderEdges(img, colorTolerance, baseColor) };

        // entirely border-colored; nothing to crop to
        if (edges.m_rowLeftEdges.empty() || edges.m_colTopEdges.empty())
            {
            return img;
            }

        const auto median = [](std::vector<std::pair<int, int>>& values) noexcept
        {
            std::nth_element(values.begin(), values.begin() + (values.size() / 2), values.end(),
                             [](const auto& lhs, const auto& rhs) noexcept
                             { return lhs.second < rhs.second; });
            return values[values.size() / 2].second;
        };

        const int left{ median(edges.m_rowLeftEdges) };
        const int right{ median(edges.m_rowRightEdges) };
        const int top{ median(edges.m_colTopEdges) };
        const int bottom{ median(edges.m_colBottomEdges) };

        if (top > bottom || left > right)
            {
            return img;
            }

        return img.GetSubImage(wxRect{ wxPoint{ left, top }, wxPoint{ right, bottom } });
        }

    //-------------------------------------------
    wxImage Image::OilPainting(const wxImage& image, const uint8_t radius /*= 2*/,
                               const float intensity /*= 20*/)
        {
        if (!image.IsOk())
            {
            return wxNullImage;
            }

        wxImage outImg{ image.Copy() };
        const auto* const imgInData = image.GetData();
        auto* const imgOutData = outImg.GetData();

        // Border pixels (depends on radius) will become black.
        // On increasing radius boundary pixels should set as black.
        std::memset(imgOutData, 0,
                    static_cast<size_t>(image.GetWidth()) * static_cast<size_t>(image.GetHeight()) *
                        3);

        // If total bytes in a row of image is not divisible by four,
        // blank bytes will be padded to the end of the row.
        // nBytesInARow bytes are the actual size of a row instead of nWidth * 3.
        // If width is 9, then actual bytes in a row will be 28, and not 27.
        const int nBytesInARow = std::ceil(image.GetWidth() * 3 / 4.0) * 4.0;

// Note that radius pixels are avoided from left, right, top, and bottom edges.
// Go to the next row of pixels...
// NOLINTBEGIN(openmp-use-default-none)
#pragma omp parallel for
        for (int nY = radius; nY < image.GetHeight() - radius; ++nY)
            {
            // ...and go across, pixel-by-pixel
            for (int nX = radius; nX < image.GetWidth() - radius; ++nX)
                {
                constexpr auto RGB_BUFFER_SIZE{ 256 };
                // Reset calculations of last pixel.
                std::array<int, RGB_BUFFER_SIZE> nIntensityCount{ 0 };
                std::array<int, RGB_BUFFER_SIZE> nSumR{ 0 };
                std::array<int, RGB_BUFFER_SIZE> nSumG{ 0 };
                std::array<int, RGB_BUFFER_SIZE> nSumB{ 0 };

                // Find intensities of nearest radius pixels in four direction.
                for (int nYO = -radius; nYO <= radius; ++nYO)
                    {
                    for (int nXO = -radius; nXO <= radius; ++nXO)
                        {
                        const int nR = imgInData[((nX + nXO) * 3) + ((nY + nYO) * nBytesInARow)];
                        const int nG =
                            imgInData[((nX + nXO) * 3) + ((nY + nYO) * nBytesInARow) + 1];
                        const int nB =
                            imgInData[((nX + nXO) * 3) + ((nY + nYO) * nBytesInARow) + 2];

                        // Find intensity of RGB value and apply intensity level.
                        const int nCurIntensity = std::clamp<int>(
                            (((nR + nG + nB) / 3.0) * intensity) / 255, 0, (RGB_BUFFER_SIZE - 1));
                        ++nIntensityCount[nCurIntensity];

                        nSumR[nCurIntensity] += nR;
                        nSumG[nCurIntensity] += nG;
                        nSumB[nCurIntensity] += nB;
                        }
                    }

                int nCurMax{ 0 };
                int nMaxIndex{ 0 };
                for (int nI = 0; nI < RGB_BUFFER_SIZE; ++nI)
                    {
                    if (nIntensityCount[nI] > nCurMax)
                        {
                        nCurMax = nIntensityCount[nI];
                        nMaxIndex = nI;
                        }
                    }

                wxASSERT_MSG(nMaxIndex >= 0 && nMaxIndex < RGB_BUFFER_SIZE,
                             L"Invalid buffer index in oil painting effect!");
                wxASSERT_MSG(((nX) * 3 + (nY)*nBytesInARow + 2) <
                                 (image.GetWidth() * image.GetHeight() * 3),
                             L"Invalid image data index in oil painting effect!");

                imgOutData[(nX * 3) + (nY * nBytesInARow)] =
                    safe_divide<int>(nSumR[nMaxIndex], nCurMax);
                imgOutData[(nX * 3) + (nY * nBytesInARow) + 1] =
                    safe_divide<int>(nSumG[nMaxIndex], nCurMax);
                imgOutData[(nX * 3) + (nY * nBytesInARow) + 2] =
                    safe_divide<int>(nSumB[nMaxIndex], nCurMax);
                }
            }
        // NOLINTEND(openmp-use-default-none)

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::Despeckle(const wxImage& image, const uint8_t radius /*= 1*/)
        {
        if (!image.IsOk())
            {
            return wxNullImage;
            }

        wxImage outImg{ image.Copy() };
        const auto* const imgInData = image.GetData();
        auto* const imgOutData = outImg.GetData();

        const int width{ image.GetWidth() };
        const int height{ image.GetHeight() };

        // derive the buffer size from the radius clamp so they can't drift out of sync
        constexpr int MAX_DESPECKLE_RADIUS{ 4 };
        constexpr size_t MAX_WINDOW_SIZE{ static_cast<size_t>(2 * MAX_DESPECKLE_RADIUS + 1) *
                                          static_cast<size_t>(2 * MAX_DESPECKLE_RADIUS + 1) };
        const int clampedRadius{ std::min<int>(radius, MAX_DESPECKLE_RADIUS) };
        const int windowSize{ (2 * clampedRadius) + 1 };
        const size_t neighborhoodCount{ static_cast<size_t>(windowSize) * windowSize };

// Border pixels (within radius) are left untouched.
// NOLINTBEGIN(openmp-use-default-none)
#pragma omp parallel for
        for (int y = clampedRadius; y < height - clampedRadius; ++y)
            {
            std::array<unsigned char, MAX_WINDOW_SIZE> redValues{ 0 };
            std::array<unsigned char, MAX_WINDOW_SIZE> greenValues{ 0 };
            std::array<unsigned char, MAX_WINDOW_SIZE> blueValues{ 0 };

            for (int x = clampedRadius; x < width - clampedRadius; ++x)
                {
                size_t index{ 0 };
                for (int yOffset = -clampedRadius; yOffset <= clampedRadius; ++yOffset)
                    {
                    for (int xOffset = -clampedRadius; xOffset <= clampedRadius; ++xOffset)
                        {
                        const size_t pixelIndex{ (static_cast<size_t>(y + yOffset) * width) +
                                                 (x + xOffset) };
                        redValues[index] = imgInData[pixelIndex * 3];
                        greenValues[index] = imgInData[(pixelIndex * 3) + 1];
                        blueValues[index] = imgInData[(pixelIndex * 3) + 2];
                        ++index;
                        }
                    }

                const auto medianPos{ neighborhoodCount / 2 };
                std::nth_element(redValues.begin(), redValues.begin() + medianPos,
                                 redValues.begin() + neighborhoodCount);
                std::nth_element(greenValues.begin(), greenValues.begin() + medianPos,
                                 greenValues.begin() + neighborhoodCount);
                std::nth_element(blueValues.begin(), blueValues.begin() + medianPos,
                                 blueValues.begin() + neighborhoodCount);

                const size_t outIndex{ (static_cast<size_t>(y) * width) + x };
                imgOutData[outIndex * 3] = redValues[medianPos];
                imgOutData[(outIndex * 3) + 1] = greenValues[medianPos];
                imgOutData[(outIndex * 3) + 2] = blueValues[medianPos];
                }
            }
        // NOLINTEND(openmp-use-default-none)

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::Sharpen(const wxImage& image, const uint8_t radius /*= 2*/,
                           const float amount /*= 1.0F*/)
        {
        if (!image.IsOk())
            {
            return wxNullImage;
            }

        const wxImage blurredImg{ image.Blur(radius) };

        wxImage outImg{ image.Copy() };
        const auto* const imgInData = image.GetData();
        const auto* const blurredData = blurredImg.GetData();
        auto* const imgOutData = outImg.GetData();

        const size_t byteCount{ static_cast<size_t>(image.GetWidth()) *
                                static_cast<size_t>(image.GetHeight()) * 3 };

        for (size_t i = 0; i < byteCount; ++i)
            {
            // unsharp mask: original + amount * (original - blurred)
            const double sharpened{ imgInData[i] + (amount * (imgInData[i] - blurredData[i])) };
            imgOutData[i] = static_cast<unsigned char>(std::clamp(sharpened, 0.0, 255.0));
            }

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::ApplyEffect(const Wisteria::ImageEffect effect, const wxImage& img)
        {
        if (effect == Wisteria::ImageEffect::Grayscale)
            {
            return img.ConvertToGreyscale();
            }
        if (effect == Wisteria::ImageEffect::BlurHorizontal)
            {
            return img.BlurHorizontal(10);
            }
        if (effect == Wisteria::ImageEffect::BlurVertical)
            {
            return img.BlurVertical(10);
            }
        if (effect == Wisteria::ImageEffect::Sepia)
            {
            return Wisteria::GraphItems::Image::Sepia(img);
            }
        if (effect == Wisteria::ImageEffect::ColorBalance)
            {
            return Wisteria::GraphItems::Image::ColorBalance(img);
            }
        if (effect == Wisteria::ImageEffect::FrostedGlass)
            {
            return Wisteria::GraphItems::Image::FrostedGlass(img);
            }
        if (effect == Wisteria::ImageEffect::OilPainting)
            {
            return Wisteria::GraphItems::Image::OilPainting(img);
            }
        if (effect == Wisteria::ImageEffect::Despeckle)
            {
            return Wisteria::GraphItems::Image::Despeckle(img);
            }
        if (effect == Wisteria::ImageEffect::Sharpen)
            {
            return Wisteria::GraphItems::Image::Sharpen(img);
            }
        return img;
        }

    //-------------------------------------------
    wxImage Image::FrostedGlass(const wxImage& image,
                                const Wisteria::Orientation orientation /*= Orientation::Both*/,
                                const uint8_t coarseness /*= 50*/)
        {
        if (!image.IsOk())
            {
            return wxNullImage;
            }

        wxImage outImg{ image.Copy() };
        const auto* const imgInData = image.GetData();
        auto* const imgOutData = outImg.GetData();
        [[maybe_unused]]
        const auto byteCount{ image.GetWidth() * image.GetHeight() * 3 };

        std::random_device dev;
        std::mt19937 twister(dev());
        std::uniform_real_distribution<> distro(0, 1);

        const auto findXInBound = [&image](const int x)
        {
            const int x1 = (x < 0)                     ? 0 :
                           (x >= image.GetWidth() * 3) ? (image.GetWidth() * 3) - 1 :
                                                         x;

            const auto stepBackToRedChannel = x1 % 3;
            return x1 - stepBackToRedChannel;
        };

        const auto findYInBound = [&image](const int y)
        {
            return (y < 0) ? 0 : (y >= image.GetHeight()) ? image.GetHeight() - 1 : y;
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
                    y = static_cast<int>(rowCounter + ((distro(twister) - 0.5) * coarseness));
                    y = findYInBound(y);
                    }

                for (auto columnCounter = 0; columnCounter < image.GetWidth() * 3;
                     columnCounter += 3)
                    {
                    int x =
                        static_cast<int>(columnCounter + ((distro(twister) - 0.5) * coarseness));

                    // generally oriented glass
                    if (orientation == Orientation::Both)
                        {
                        y = static_cast<int>(rowCounter + ((distro(twister) - 0.5) * coarseness));
                        y = findYInBound(y);
                        }

                    x = findXInBound(x);

                    // source pixel
                    auto w1 = (image.GetWidth() * 3 * y) + x;
                    wxASSERT_MSG(w1 + 2 < byteCount, L"Invalid index in image buffer!");
                    const auto r = imgInData[w1];
                    const auto g = imgInData[w1 + 1];
                    const auto b = imgInData[w1 + 2];

                    // target pixel
                    w1 = w2 + columnCounter;
                    wxASSERT_MSG(w1 + 2 < byteCount, L"Invalid index in image buffer!");
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
                const auto x = findXInBound(
                    static_cast<int>(columnCounter + ((distro(twister) - 0.5) * coarseness)));
                for (auto rowCounter = 0; rowCounter < image.GetHeight(); ++rowCounter)
                    {
                    const auto y = findYInBound(
                        static_cast<int>(rowCounter + ((distro(twister) - 0.5) * coarseness)));

                    // Source pixel
                    auto w1 = (image.GetWidth() * 3 * y) + x;
                    wxASSERT_MSG(w1 + 2 < byteCount, L"Invalid index in image buffer!");
                    const auto r = imgInData[w1];
                    const auto g = imgInData[w1 + 1];
                    const auto b = imgInData[w1 + 2];

                    // Target pixel
                    w1 = image.GetWidth() * 3 * rowCounter + columnCounter;
                    wxASSERT_MSG(w1 + 2 < byteCount, L"Invalid index in image buffer!");
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
            {
            return wxNullImage;
            }

        wxImage outImg{ image.Copy() };
        const auto* const imgInData = image.GetData();
        auto* const imgOutData = outImg.GetData();

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
            const auto intensity = (0.3 * r) + (0.6 * g) + (0.1 * b);

            // Red
            auto tone = (intensity > threshold) ? 255.0 : intensity + 255.0 - threshold;
            const auto dRed = tone;

            // Green
            tone = (intensity > thres6By7) ? 255.0 : intensity + 255.0 - thres6By7;
            auto dGreen = tone;

            // Blue
            tone = (intensity < thres6) ? 0 : intensity - thres6;
            auto dBlue = tone;

            tone = thres7;
            dGreen = std::max(dGreen, tone);
            dBlue = std::max(dBlue, tone);

            imgOutData[index] = static_cast<unsigned char>(std::clamp(dRed, 0.0, 255.0));
            imgOutData[index + 1] = static_cast<unsigned char>(std::clamp(dGreen, 0.0, 255.0));
            imgOutData[index + 2] = static_cast<unsigned char>(std::clamp(dBlue, 0.0, 255.0));
            }

        return outImg;
        }

    //-------------------------------------------
    wxImage Image::ColorBalance(const wxImage& image, const uint8_t percentTrim /*= 1*/)
        {
        if (!image.IsOk())
            {
            return wxNullImage;
            }

        wxImage outImg{ image.Copy() };
        const auto* const imgInData = image.GetData();
        auto* const imgOutData = outImg.GetData();

        const size_t pixelCount =
            static_cast<size_t>(image.GetWidth()) * static_cast<size_t>(image.GetHeight());
        const size_t byteCount = pixelCount * 3;

        constexpr size_t RGB_BUFFER_SIZE{ 256 };
        std::array<size_t, RGB_BUFFER_SIZE> redHistogram{ 0 };
        std::array<size_t, RGB_BUFFER_SIZE> greenHistogram{ 0 };
        std::array<size_t, RGB_BUFFER_SIZE> blueHistogram{ 0 };

        for (size_t index = 0; index < byteCount; index += 3)
            {
            ++redHistogram[imgInData[index]];
            ++greenHistogram[imgInData[index + 1]];
            ++blueHistogram[imgInData[index + 2]];
            }

        // For a channel's histogram, find the low and high values that clip off
        // percentTrim% of pixels from the dark and light ends, respectively.
        const auto clipCount =
            static_cast<size_t>(pixelCount * (std::clamp<uint8_t>(percentTrim, 0, 49) / 100.0));
        const auto findLowHigh = [clipCount](const std::array<size_t, RGB_BUFFER_SIZE>& histogram)
        {
            uint8_t low{ 0 };
            size_t runningCount{ 0 };
            for (size_t i = 0; i < RGB_BUFFER_SIZE; ++i)
                {
                runningCount += histogram[i];
                if (runningCount > clipCount)
                    {
                    low = static_cast<uint8_t>(i);
                    break;
                    }
                }

            uint8_t high{ 255 };
            runningCount = 0;
            for (size_t i = RGB_BUFFER_SIZE; i > 0; --i)
                {
                runningCount += histogram[i - 1];
                if (runningCount > clipCount)
                    {
                    high = static_cast<uint8_t>(i - 1);
                    break;
                    }
                }

            return std::make_pair(low, high);
        };

        const auto [redLow, redHigh] = findLowHigh(redHistogram);
        const auto [greenLow, greenHigh] = findLowHigh(greenHistogram);
        const auto [blueLow, blueHigh] = findLowHigh(blueHistogram);

        const auto stretch = [](const unsigned char value, const uint8_t low, const uint8_t high)
        {
            if (high <= low)
                {
                return value;
                }
            const double stretched = safe_divide<double>((static_cast<double>(value) - low) * 255.0,
                                                         static_cast<double>(high) - low);
            return static_cast<unsigned char>(std::clamp(stretched, 0.0, 255.0));
        };

        for (size_t index = 0; index < byteCount; index += 3)
            {
            imgOutData[index] = stretch(imgInData[index], redLow, redHigh);
            imgOutData[index + 1] = stretch(imgInData[index + 1], greenLow, greenHigh);
            imgOutData[index + 2] = stretch(imgInData[index + 2], blueLow, blueHigh);
            }

        return outImg;
        }
    } // namespace Wisteria::GraphItems
