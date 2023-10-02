/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden (portions from Bhumika Thatte, Raghavendra Sri, Prasad R V, and Avijnata)
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.
     (Some portions are under Code Project Open License (CPOL) 1.02, where noted.)

     SPDX-License-Identifier: BSD-3-Clause, CPOL-1.02
@{*/

#ifndef __WISTERIA_GRAPHIMAGE_H__
#define __WISTERIA_GRAPHIMAGE_H__

#include <cstring>
#include <random>
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/statline.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/bmpbndl.h>
#include <wx/xml/xml.h>
#include <wx/regex.h>
#if __has_include(<omp.h>)
    #include <omp.h>
#endif
#include "graphitems.h"
#include "colorbrewer.h"
#include "../math/mathematics.h"
#include "../util/memorymappedfile.h"
#include "../easyexif/exif.h"

// forward declares
namespace Wisteria
    {
    class Canvas;
    }

namespace Wisteria::UI
    {
    class Thumbnail;
    }

namespace Wisteria::GraphItems
    {
    class ShapeRenderer;
    class FillableShape;
    }

namespace Wisteria::GraphItems
    {
    /** @brief An image that can be placed on a graph.
        @details Also includes image loading and effect functions. For example,
            LoadFile() will load a JPEG and adjust its orientation (if necessary).

            Other features include creating silhouettes, filling an area with a stipple bitmap,
            stitching multiple images together, changing pixel colors,
            changing the opacity, etc.
        @sa The [image](../../Images.md) overview for more information.*/
    class Image final : public GraphItems::GraphItemBase
        {
        friend class Wisteria::Canvas;
        friend class Wisteria::UI::Thumbnail;
        friend class Wisteria::GraphItems::ShapeRenderer;
        friend class Wisteria::GraphItems::FillableShape;
    public:
        /// @private
        Image()
            {
            SetOk(false);
            GetPen() = wxNullPen;
            }
        /** @brief Constructor.
            @param itemInfo Base information for the plot object.
            @param img The image to render.
                LoadFile() can be used as a quick way to load an image here.*/
        Image(const GraphItems::GraphItemInfo& itemInfo,
              const wxImage& img) : GraphItemBase(itemInfo),
            m_originalImg(img), m_img(img),
            m_size(img.GetSize()),
            m_frameSize(img.GetSize())
            { SetOk(m_originalImg.IsOk()); }
        /** @brief Constructor.
            @param img The image to render.*/
        explicit Image(const wxImage& img) :
            m_originalImg(img), m_img(img),
            m_size(img.GetSize()),
            m_frameSize(img.GetSize())
            {
            SetOk(m_originalImg.IsOk());
            GetPen() = wxNullPen;
            }
        /** @brief Constructor that takes an image path.
            @param imgPath The image filepath to load.
            @note This will call LoadFile(), which will make corrections
                such as adjusting the orientation in JPEG files.*/
        explicit Image(const wxString& imgPath)
            {
            const auto img = LoadFile(imgPath);
            if (img.IsOk())
                {
                m_originalImg = m_img = img;
                m_size = img.GetSize();
                m_frameSize = img.GetSize();
                }
            SetOk(m_originalImg.IsOk());
            GetPen() = wxNullPen;
            }

        /// @brief Sets the image to a null image.
        void Clear()
            {
            m_originalImg = m_img = wxNullImage;
            m_frameSize = m_size = wxDefaultSize;
            SetOk(false);
            }

        /// @returns The original image that was loaded into this object.
        /// @note Because this is the original image, any size or opacity
        ///     changes being used for this object will not be applied to this image.
        [[nodiscard]]
        const wxImage& GetOriginalImage() const noexcept
            { return m_originalImg; }

        /** @name Image Loading Functions
            @brief Functions related to querying and importing images.
            @details These are all static functions and the result of most of them
                are `wxImage` objects that can be passed to an `Image`'s constructor.*/
        /// @{

        /** @brief Gets the default size of an SVG file.
            @details This is useful for determining the aspect ratio of an SVG file.
                This can be passed to a @c wxBitmapBundle when it loads an SVG.
            @param filePath The file path to the SVG file.
            @returns The default size of the SVG. (Will be a 32x32 size if the
                size can't be read.)
            @note The units (e.g., mm or px) in the SVG are ignored, only the numeric
                values of the sizes are read.*/
        [[nodiscard]]
        static wxSize GetSVGSize(const wxString& filePath);
        /** @brief Either downscales or upscales a size to another, maintaining the
                original's aspect ratio.
            @param originalSz The original size.
            @param suggestedSz The new size.
            @returns The original size, either upscaled or downscaled to fit within
                the suggested size, with the original's aspect ratio maintained.*/
        [[nodiscard]]
        static wxSize ToBestSize(const wxSize originalSz, const wxSize suggestedSz);
        /** @returns A bitmap type from a file extension.
            @param[in,out] ext The file extension to review
                (can be either the extension or full file path).
                If a full filepath is used, then this will be returned as just the extension.
            @note SVG files will return @c wxBITMAP_TYPE_ANY, so check @c ext to further
                review the file type.*/
        [[nodiscard]]
        static wxBitmapType GetImageFileTypeFromExtension(wxString& ext);
        /** @brief Loads image and adjusts its JPEG orientation (if necessary).
            @param filePath The filepath of the image to load.
            @note Memory mapping is used when loading, which can help memory usage when
                large files.
            @returns The image loaded from @c filePath.*/
        [[nodiscard]]
        static wxImage LoadFile(const wxString& filePath);
        /// @}

        /** @name Image Editing Functions
            @brief Functions related to editing images.
            @details These are all static functions and the result of most of them
                are `wxImage` objects that can be passed to an `Image`'s constructor.*/
        /// @{

        /** @brief Fits an image to a rect, cropping it evenly if necessary.
            @details For example, if the height of the image is closer to the rect's than
                the width is, then its height will be scaled to the rect's height
                (while maintaining the aspect ratio).\n
            @param img The image to fit.
            @param rect The rect to crop the image to.
            @param centerImage @c true to center the image if it needs cropping.
            @returns The cropped image.*/
        [[nodiscard]]
        static wxImage CropImageToRect(const wxImage& img, const wxRect rect,
                                       const bool centerImage);
        /** @brief Combines a list of images together, going from left-to-right.
            @param images The images (a @c vector of `wxImage`s or `wxBitmap`s) to stitch.
            @returns The combined image.
            @sa StitchVertically().
            @code
             // set the background from multiple images
             canvas->SetBackgroundImage(
                     Image(
                     Image::StitchHorizontally({
                         Image::LoadFile(L"C:\\Pictures\\IMG_0517.JPG"),
                         Image::LoadFile(L"C:\\Pictures\\IMG_0592.JPG"),
                         Image::LoadFile(L"C:\\Pictures\\IMG_1288.JPG")
                         }))
                 );
            @endcode*/
        template<typename T>
        [[nodiscard]]
        static wxImage StitchHorizontally(const std::vector<T>& images)
            {
            if (images.size() == 0)
                { return wxNullImage; }
            const int imgWidth = std::accumulate(images.cbegin(), images.cend(),
                0.0f,
                [](const auto initVal, const auto& img) noexcept
                  { return initVal + img.GetWidth(); });
            const auto maxHeightImg = std::max_element(images.cbegin(), images.cend(),
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
        /** @brief Combines a list of images together, going from top-to-bottom.
            @param images The images (a @c vector of `wxImage`s or `wxBitmap`s) to stitch.
            @returns The combined image.
            @sa StitchHorizontally().*/
        template<typename T>
        [[nodiscard]]
        static wxImage StitchVertically(const std::vector<T>& images)
            {
            if (images.size() == 0)
                { return wxNullImage; }
            const int imgHeight = std::accumulate(images.cbegin(), images.cend(),
                0.0f,
                [](const auto initVal, const auto& img) noexcept
                  { return initVal + img.GetHeight(); });
            const auto maxWidthImg = std::max_element(images.cbegin(), images.cend(),
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
        /** @brief Renders a repeating bitmap across another bitmap's area.
            @param stipple The bitmap to draw repeatedly within the output image.
            @param fillSize The size of the output image to create.
            @param direction The direction repeating bitmaps.
            @param includeShadow Whether to draw a shadow under the bitmaps.
            @param shadowSize The width/height of the shadow from the image. Value should be
                scaled for canvas scaling and DPI.
            @returns The image with the stipple drawn across it.*/
        [[nodiscard]]
        static wxImage CreateStippledImage(wxImage stipple, const wxSize fillSize,
                                           const Orientation direction,
                                           const bool includeShadow,
                                           const wxCoord shadowSize);
        /** @brief Creates a silhouette (all black copy) of an image.
            @param image The image to create the silhouette from.
            @param opaque Whether the silhouette should be a fully opaque shadow.
                If @c false, then it will be translucent.
            @returns The silhouette of the image.*/
        [[nodiscard]]
        static wxImage CreateSilhouette(const wxImage& image,
                                        const bool opaque = true);
        /** @brief Creates a copy of an image with a color filter applied across it.
            @details For example, applying a light blue will make the image look
                "cooler," while applying an orange or red filter will make it
                appear "warmer."
            @param image The image to create the color-filtered image from.
            @param color The color to fill the image with.
            @param opacity The opacity of the color to fill the image with.
            @returns The color-filtered of the image.*/
        [[nodiscard]]
        static wxImage CreateColorFilteredImage(const wxImage& image,
                                                const wxColour color,
                                                const uint8_t opacity = 100);
        /** @private
            @brief Renders a glassy surface across a box.
            @param fillSize The size of the output image to create.
            @param color The base color to fill the box with.
            @param direction The direction of the glassy shine.
            @returns The glassy image.*/
        [[deprecated("Use Polygon::SetShape() with GlassyRectangle instead.")]]
        [[nodiscard]]
        static wxImage CreateGlassEffect(const wxSize fillSize, const wxColour color,
                                         const Orientation direction);
        /** @brief Sets the opacity of a bitmap.
            @param bmp The bitmap to edit.
            @param opacity The opacity to set the bitmap to.
            @param preserveTransparentPixels Set to @c true to not alter pixels
                that are already transparent in the image.*/
        static void SetOpacity(wxBitmap& bmp, const uint8_t opacity,
            const bool preserveTransparentPixels = false);
        /** @brief Sets the opacity of an image.
            @param image The image to edit.
            @param opacity The opacity to set the bitmap to.
            @param preserveTransparentPixels Set to @c true to not alter pixels that
                are already transparent in the image.*/
        static void SetOpacity(wxImage& image, const uint8_t opacity,
                               const bool preserveTransparentPixels);
        /** @brief Sets the opacity of a bitmap.
            @param image The image to edit.
            @param opacity The opacity to set the bitmap to.
            @param colorToPreserve Color which will not have its opacity altered.
                This is useful for preserving highlights in an image.*/
        static void SetOpacity(wxImage& image, const uint8_t opacity,
                               const wxColour colorToPreserve);
        /** @brief Sets the opacity of a bitmap.
            @param bmp The bitmap to edit.
            @param opacity The opacity to set the bitmap to.
            @param colorToPreserve Color which will not have its opacity altered.
                This is useful for preserving highlights in an image.*/
        static void SetOpacity(wxBitmap& bmp, const uint8_t opacity,
                               const wxColour colorToPreserve);
        /** @brief Set the specified color in an image to transparent.
            @details Any pixel of this color will be set to transparent in the alpha channel.
            @param image The image to edit.
            @param color The color to set to transparent.*/
        static void SetColorTransparent(wxImage& image, const wxColour color);
        /** @brief Changes each pixel of a given color with another one in a provided image,
                and returns the altered image.
            @param image The original image.
            @param srcColor The color to replace.
            @param destColor The color to replace it with.
            @returns The altered image.*/
        [[nodiscard]]
        static wxImage ChangeColor(const wxImage& image, const wxColour srcColor,
                                   const wxColour destColor);
        /** @brief Applies an oil painting effect to an image.
            @param image The original image.
            @param radius 'For each pixel, a number of pixels around that pixel are taken into account.\n
                The radius simply defines how many pixels in each direction to look for. A radius of 5,
                for example, should be good for rough oil painting pictures.'
            @param intensity 'For this algorithm, each pixel will be put into an intensity bin.
                The true intensity of a pixel is defined as (r+b+g)/3, and can range anywhere from 0 to 256.
                However, oil paintings have a much more blocky effect, so each pixel will have its
                intensity binned. For a fairly blocky oil painting, 20 is a good reference number.\n
                Note that the default here is 40 to produce a more pronounced effect.
            @returns The oil-painted image.
            @note Adapted from https://www.codeproject.com/articles/471994/oilpainteffect,
                by author Santhosh G_. Santhosh G_'s code was based on
                http://supercomputingblog.com/graphics/oil-painting-algorithm/, which is also the
                source for the explanations for @c radius and @c intensity.\n
                This article is licensed under the The Code Project Open License (CPOL) 1.02.*/
        [[nodiscard]]
        static wxImage OilPainting(const wxImage& image, const uint8_t radius = 10,
                                   const float intensity = 40);
        /** @brief Applies a sepia (i.e., faded photograph) effect to an image.
            @param image The original image.
            @param magnitude The strength of the effect.
            @returns The sepia-toned image.
            @note Adapted from https://www.codeproject.com/articles/996192/some-cool-image-effects,
                by Bhumika Thatte, Raghavendra Sri, Prasad R V, and Avijnata.\n
                This article is licensed under the The Code Project Open License (CPOL) 1.02.*/
        [[nodiscard]]
        static wxImage Sepia(const wxImage& image, const uint8_t magnitude = 75);
        /** @brief Applies a frosted glass window effect to an image.
            @param image The original image.
            @param orientation The direction(s) of the frosting effect.
            @param coarseness The strength of the effect.
            @returns The image, as it may appear when viewed through frosted glass.
            @note Adapted from https://www.codeproject.com/articles/996192/some-cool-image-effects,
                by Bhumika Thatte, Raghavendra Sri, Prasad R V, and Avijnata.\n
                The authors state that this algorithm is
                'based on the Lecture Notes by Prof. Onur Guleryuz of Polytechnic University, New York.'\n
                This article is licensed under the The Code Project Open License (CPOL) 1.02.*/
        [[nodiscard]]
        static wxImage FrostedGlass(const wxImage& image,
            const Wisteria::Orientation orientation = Orientation::Both, const uint8_t coarseness = 50);
        /** @returns Returns an image with an effect applied to it.
            @param effect The effect to apply.
            @param img The basis image to apply the effect to.*/
        [[nodiscard]]
        static wxImage ApplyEffect(const Wisteria::ImageEffect effect, const wxImage& img);
        /// @}

        /** @name Size Functions
            @brief Functions related to the image's size.
            @details These are useful for changing the dimensions of the image while maintaining
                its aspect ratio.*/
        /// @{

        /** @brief Sets the image's width.
            @param width The new width.
            @note The aspect ratio of the image will be maintained.*/
        void SetWidth(const wxCoord width);
        /** @brief Sets the image's height.
            @param height The new height.
            @note The aspect ratio of the image will be maintained.*/
        void SetHeight(const wxCoord height);
        /** @brief Explicitly sets the image's size.
            @param sz The new size.
            @warning The image will be stretched to fit in this size, distorting its appearance.\n
                Prefer using SetWidth(), SetHeight(), or SetBestSize() to maintain aspect ratio.*/
        void SetSize(const wxSize sz);
        /** @brief Sets the image's size to fit inside of the specified bounding box.
            @param suggestedSz The suggested bounding box size.
            @returns The new size of the control based on the suggested size.
            @note The image's new size may be different from the suggested size here, as it will
                maintain the image's aspect ratio.*/
        wxSize SetBestSize(const wxSize suggestedSz);
        /// @returns How the image's size is adjusted when its bounding box is changed.
        [[nodiscard]]
        ResizeMethod GetResizeMethod() const noexcept
            { return m_resizeMethod; }
        /** @brief Sets how the image's size is adjusted when its bounding box is changed.
            @details The default it to either upscale or downscale the image as necessary,
                which will lead to fidelity loss.
            @param resizeMethod How to resize the image.*/
        void SetResizeMethod(ResizeMethod resizeMethod) noexcept
            { m_resizeMethod = resizeMethod; }
        /// @}

        /** @name Image Editing Functions
            @brief Functions related to editing the image.*/
        /// @{

        /** @brief Sets the opacity of the image.
            @param opacity The opacity to set the image to.*/
        void SetOpacity(const uint8_t opacity)
            { m_opacity = opacity; }
        /// @} 
private:
        /// @returns The size of the image as it is being drawn.
        [[nodiscard]]
        const wxSize& GetImageSize() const noexcept
            { return m_size; }
        /** @returns The frame that the image may be getting centered in.
            @note This only applies with a call to SetBoundingBox(), which would force
                the image to fit to a specific size.\n
                If this size differs from aspect ratio that the image needs to maintain,
                then the image will be positioned into this box.
            @sa SetPageVerticalAlignment() and SetPageHorizontalAlignment() for controlling
                this positioning.*/
        [[nodiscard]]
        const wxSize& GetFrameSize() const noexcept
            { return m_frameSize; }
        /** @brief Draws the image onto the given DC.
            @param dc The DC to render onto.
            @returns The box that the image is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns The rectangle on the canvas where the image would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]]
        wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
        /** @brief Bounds the image to the given rectangle.
            @param rect The rectangle to bound the image to.
            @param parentScaling This parameter is ignored.
            @note The size of the image will be adjusted (height-wise first,
                then length-wise if necessary) to this box.
                If the image isn't wide enough to fill the bounding box,
                then it will be anchored within the specified rectangle.\n
                Call SetPageVerticalAlignment() and SetPageHorizontalAlignment()
                to control how it is positioned inside of its bounding box.*/
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final;
        /** @brief Moves the item by the specified x and y values.
            @param xToMove the amount to move horizontally.
            @param yToMove the amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) noexcept final
            { SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove)); }
        /** @returns @c true if the given point is inside of the image.
            @param pt The point to check.*/
        [[nodiscard]]
        bool HitTest(const wxPoint pt, wxDC& dc) const noexcept final
            { return GetBoundingBox(dc).Contains(pt); }
        /// @brief Helper for calling downscaled_size().
        /// @param sz The size to convert.
        /// @returns The @c wxSize object, wrapped into a `std::pair`.
        [[nodiscard]]
        inline static auto wxSizeToPair(const wxSize sz) noexcept
            { return std::make_pair(sz.GetWidth(), sz.GetHeight()); }
        wxImage m_originalImg;
        mutable wxImage m_img;
        wxSize m_size{ 0, 0 };
        wxSize m_frameSize{ 0, 0 };
        uint8_t m_opacity{ wxALPHA_OPAQUE };
        ResizeMethod m_resizeMethod{ ResizeMethod::DownscaleOrUpscale };
        };
    }

namespace Wisteria::Images::Schemes
    {
    /// @brief Class for a list of images.
    class ImageScheme
        {
    public:
        /// @brief Constructor.
        /// @param images The vector of images to fill the scheme with.
        explicit ImageScheme(const std::vector<wxBitmapBundle>& images) :
            m_images(images)
            {}
        /// @private
        explicit ImageScheme(std::vector<wxBitmapBundle>&& images) :
            m_images(std::move(images))
            {}
        /// @brief Constructor.
        /// @param images The initializer list of images to fill the scheme with.
        explicit ImageScheme(const std::initializer_list<wxBitmapBundle>& images) :
            m_images(images)
            {}
        /// @returns The list of images from the scheme.
        [[nodiscard]]
        const std::vector<wxBitmapBundle>& GetImages() const noexcept
            { return m_images; }
        /// @private
        [[nodiscard]]
        std::vector<wxBitmapBundle>& GetImages() noexcept
            { return m_images; }
        /** @returns The image at the given index.
            @param index The index into the image list to return. If index is outside
                the number of images, then it will recycle (i.e., wrap around).
                For example, if there are 2 images, index 1 will return 1;
                however, index 2 will wrap around and return image 0 and
                index 3 will return image 1.*/
        [[nodiscard]]
        const wxBitmapBundle& GetImage(const size_t index) const noexcept
            {
            return (m_images.size() == 0) ?
                m_emptyImage :
                m_images.at(index % m_images.size());
            }
        /** @brief Adds an image to the scheme.
            @param image The image to add.*/
        void AddImage(const wxBitmapBundle& image)
            { m_images.push_back(image); }
        /// @private
        void AddImage(wxBitmapBundle&& image)
            { m_images.push_back(image); }
        /// @brief Removes all images from the collection.
        void Clear() noexcept
            { m_images.clear(); }
    private:
        std::vector<wxBitmapBundle> m_images;
        wxBitmapBundle m_emptyImage;
        };
    }

/** @}*/

#endif //__WISTERIA_GRAPHIMAGE_H__
