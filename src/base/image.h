/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_GRAPHIMAGE_H__
#define __WISTERIA_GRAPHIMAGE_H__

#include <cstring>
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/statline.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/bmpbndl.h>
#include "graphitems.h"
#include "colorbrewer.h"
#include "../math/mathematics.h"
#include "../util/memorymappedfile.h"
#include "../easyexif/exif.h"
#include "../nanosvg/src/nanosvg.h"

// forward declares
namespace Wisteria::UI
    {
    class Canvas;
    }

namespace Wisteria::UI
    {
    class Thumbnail;
    }

namespace Wisteria::Graphs
    {
    class Graph2D;
    }

namespace Wisteria::GraphItems
    {
    /** @brief An image that can be placed on a graph.
        @details Also includes image loading and effect functions. For example,
         LoadImageWithCorrection() will load a JPEG and adjust its orientation (if necessary).

         Other features include creating silhouettes, drawing a glassy effect,
         filling an area with a stipple bitmap, stitching multiple images together,
         changing pixel colors, changing the opacity, etc.
        @sa The [image](../../Images.md) overview for more information.*/
    class Image final : public GraphItems::GraphItemBase
        {
        friend class Graphs::Graph2D;
        friend class Wisteria::Canvas;
        friend class Wisteria::UI::Thumbnail;
    public:
        /// @private
        Image()
            { SetOk(false); }
        /** @brief Constructor.
            @param itemInfo Base information for the plot object.
            @param img The image to render. LoadImageWithCorrection() can be used as a quick way to load an image here.*/
        Image(const GraphItems::GraphItemInfo& itemInfo,
            const wxImage& img) : GraphItemBase(itemInfo),
            m_originalImg(img), m_img(img),
            m_size(img.GetSize()),
            m_frameSize(img.GetSize())
            {
            SetOk(m_originalImg.IsOk());
            }
        /** @brief Constructor.
            @param img The image to render..*/
        explicit Image(const wxImage& img) :
            m_originalImg(img), m_img(img),
            m_size(img.GetSize()),
            m_frameSize(img.GetSize())
            {
            SetOk(m_originalImg.IsOk());
            }
        /** @brief Constructor.
            @param imgPath The image filepath to load.
            @note This will call LoadImageWithCorrection(), which will make corrections such as adjusting
             the orientation in JPEG files.*/
        explicit Image(const wxString& imgPath)
            {
            const auto img = LoadImageWithCorrection(imgPath);
            if (img.IsOk())
                {
                m_originalImg = m_img = img;
                m_size = img.GetSize();
                m_frameSize = img.GetSize();
                }
            SetOk(m_originalImg.IsOk());
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
        ///  changes being used for this object will not be applied to this image.
        [[nodiscard]] const wxImage& GetOriginalImage() const noexcept
            { return m_originalImg; }

        /** @name Image Loading Functions
            @brief Functions related to querying, editing, and importing images.
            @details These are all static functions and the result of most of them
             are `wxImage` objects that can be passed to an Image's constructor.*/
        /// @{

        /** @brief Gets the default size of an SVG file.
            @details This is useful for determining the aspect ratio of an SVG file.
             This can be passed to a `wxBitmapBundle` when it loads an SVG.
            @param filePath The file path to the SVG file.
            @returns The default size of the SVG. Will be an invalid size if the file fails to load.*/
        [[nodiscard]] static wxSize GetSVGSize(const wxString& filePath);

        /** @returns A bitmap type from a file extension.
            @param[in,out] ext The file extension to review (can be either the extension or full file path).
             If a full filepath is used, then this will be returned as just the extension.
            @note SVG files will return @c wxBITMAP_TYPE_ANY, so check @c ext to further review the file type.*/
        [[nodiscard]] static wxBitmapType GetImageFileTypeFromExtension(wxString& ext);
        /** @brief Loads image and adjusts its JPEG orientation (if necessary).
            @param filePath The filepath of the image to load.
            @note Memory mapping is used when loading, which can help memory usage when opening large files.
            @returns The image loaded from @c filePath.*/
        [[nodiscard]] static wxImage LoadImageWithCorrection(const wxString& filePath);
        /** @brief Combines a list of images together, going from left-to-right.
            @param images The images to stitch.
            @returns The combined image.
            @sa StitchVertically().
            @code
             // set the background from multiple images
             canvas->SetBackgroundImage(
                     Image(
                     Image::StitchHorizontally({
                         Image::LoadImageWithCorrection(L"C:\\Pictures\\IMG_0517.JPG"),
                         Image::LoadImageWithCorrection(L"C:\\Pictures\\IMG_0592.JPG"),
                         Image::LoadImageWithCorrection(L"C:\\Pictures\\IMG_1288.JPG")
                         }))
                 );
            @endcode*/
        [[nodiscard]] static wxImage StitchHorizontally(const std::initializer_list<wxImage>& images);
        /** @brief Combines a list of images together, going from top-to-bottom.
            @param images The images to stitch.
            @returns The combined image.
            @sa StitchHorizontally().*/
        [[nodiscard]] static wxImage StitchVertically(const std::initializer_list<wxImage>& images);
        /** @brief Renders a repeating bitmap across another bitmap's area.
            @param stipple The bitmap to draw repeatedly within the output image.
            @param fillSize The size of the output image to create.
            @param direction The direction repeating bitmaps.
            @param includeShadow Whether to draw a shadow under the bitmaps.
            @param shadowSize The width/height of the shadow from the image. Value should be
             scaled for canvas scaling and DPI.
            @returns The image with the stipple drawn across it.*/
        [[nodiscard]] static wxImage CreateStippledImage(wxImage stipple, const wxSize fillSize,
            const Orientation direction, const bool includeShadow,
            const wxCoord shadowSize);
        /** @brief Creates a silhouette (all black copy) of an image.
            @param image The image to create the silhouette from.
            @param opaque Whether the silhouette should be a fully opaque shadow.
             If `false`, then it will be translucent.
            @returns The silhouette of the image.*/
        [[nodiscard]] static wxImage CreateSilhouette(const wxImage& image, const bool opaque = true);
        /** @brief Renders a glassy surface across a box.
            @param fillSize The size of the output image to create.
            @param color The base color to fill the box with.
            @param direction The direction of the glassy shine.
            @returns The glassy image.*/
        [[nodiscard]] static wxImage CreateGlassEffect(const wxSize fillSize, const wxColour color,
                                                       const Orientation direction);
        /** @brief Changes each pixel of a given color with another one in a given image,
             and returns the corrected image.
            @param image The original image.
            @param srcColor The color to replace.
            @param destColor The color to replace it with.
            @returns The altered image.*/
        [[nodiscard]] static wxImage ChangeColor(const wxImage& image, const wxColour srcColor, const wxColour destColor);
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
            @warning The image will be stretched to fit in this size, distorting its appearance.
             Prefer using SetWidth(), SetHeight(), or SetBestSize() to maintain the aspect ratio.*/
        void SetSize(const wxSize sz);
        /** @brief Sets the image's size to fit inside of the specified bounding box.
            @param suggestedSz The suggested bounding box size.
            @returns The new size of the control based on the suggested size.
            @note The image's new size may be different from the suggested size here, as it will
             maintain the image's aspect ratio.*/
        wxSize SetBestSize(const wxSize suggestedSz);
        /// @}

        /** @name Image Effect Functions
            @brief Functions related to applying effects to the image.*/
        /// @{

        /** @brief Sets the opacity of the image.
            @param opacity The opacity to set the image to.*/
        void SetOpacity(const uint8_t opacity)
            { m_opacity = opacity; }
        /// @}
    private:
        /** @brief Set the specified color in an image to transparent.
            @details Any pixel of this color will be set to transparent in the alpha channel.
            @param image The image to edit.
            @param color The color to set to transparent.*/
        static void SetColorTransparent(wxImage& image, const wxColour color);
        /** @brief Sets the opacity of a bitmap.
            @param bmp The bitmap to edit.
            @param opacity The opacity to set the bitmap to.
            @param preserveTransparentPixels Set to `true` to not alter pixels that are already transparent in the image.*/
        static void SetOpacity(wxBitmap& bmp, const uint8_t opacity, const bool preserveTransparentPixels = false);
        /** @brief Sets the opacity of an image.
            @param image The image to edit.
            @param opacity The opacity to set the bitmap to.
            @param preserveTransparentPixels Set to `true` to not alter pixels that are already transparent in the image.*/
        static void SetOpacity(wxImage& image, const uint8_t opacity, const bool preserveTransparentPixels);
        /// @returns The size of the image as it is being drawn.
        [[nodiscard]] const wxSize& GetImageSize() const noexcept
            { return m_size; }
        /** @returns The frame that the image may be getting centered in.
            @note This only applies with a call to SetBoundingBox(), which would force the image to fit to a specific size.
             If this size differs from aspect ratio that the image needs to maintain, then the image will be anchored into this box.
            @sa SetAnchoring().*/
        [[nodiscard]] const wxSize& GetFrameSize() const noexcept
            { return m_frameSize; }
        /** @brief Draws the image onto the given wxDC.
            @param dc The wxDC to render onto.
            @returns The box that the image is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
        /** @brief Bounds the image to the given rectangle.
            @param rect The rectangle to bound the image to.
            @param parentScaling This parameter is ignored.
            @note The size of the image will be adjusted (height-wise first, then length-wise if necessary) to this box.
             If the image isn't wide enough to fill the bounding box, then it will be anchored within the specified rectangle.
             Call SetAnchoring() to control how it is anchored. SetScaling() must be called prior to calling this to ensure proper resizing calculations.*/
        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) final;
        /** @brief Moves the item by the specified x and y values.
            @param xToMove the amount to move horizontally.
            @param yToMove the amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) noexcept final
            { SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove)); }
        /** @returns `true` if the given point is inside of the image.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const noexcept final
            { return GetBoundingBox(dc).Contains(pt); }
        /// @brief Helper for calling calculate_downscaled_size().
        /// @param sz The size to convert.
        /// @returns The wxSize object, wrapped into a std::pair.
        [[nodiscard]] inline static auto wxSizeToPair(const wxSize sz) noexcept
            { return std::make_pair(sz.GetWidth(), sz.GetHeight()); }
        wxImage m_originalImg;
        mutable wxImage m_img;
        wxSize m_size{ 0, 0};
        wxSize m_frameSize{ 0, 0 };
        uint8_t m_opacity{ wxALPHA_OPAQUE };
        };
    }

/** @}*/

#endif //__WISTERIA_GRAPHIMAGE_H__
