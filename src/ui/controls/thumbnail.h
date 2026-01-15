/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_THUMBNAIL_H
#define WISTERIA_THUMBNAIL_H

#include "../../base/image.h"
#include <wx/bitmap.h>
#include <wx/dcbuffer.h>
#include <wx/dialog.h>
#include <wx/dnd.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/wx.h>

/// @cond DOXYGEN_IGNORE
wxDECLARE_EVENT(wxEVT_THUMBNAIL_CHANGED, wxCommandEvent);

#define EVT_THUMBNAIL_CHANGED(winId, fn)                                                           \
    wx__DECLARE_EVT1(wxEVT_THUMBNAIL_CHANGED, winId, wxCommandEventHandler(fn))

/// @endcond

namespace Wisteria::UI
    {
    /// @brief Helper class to show a thumbnail fullscreen.
    /// @private
    class EnlargedImageWindow : public wxDialog
        {
      public:
        EnlargedImageWindow(wxBitmap bitmap, wxWindow* parent, wxWindowID id = wxID_ANY,
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP |
                                         wxFULL_REPAINT_ON_RESIZE);
        EnlargedImageWindow() = delete;
        EnlargedImageWindow(const EnlargedImageWindow&) = delete;
        EnlargedImageWindow& operator=(const EnlargedImageWindow&) = delete;

        void OnPaint([[maybe_unused]] wxPaintEvent& event);
        void OnClick(const wxMouseEvent& event);
        void OnChar([[maybe_unused]] wxKeyEvent& event);

        void SetBitmap(const wxBitmap& bitmap) noexcept { m_bitmap = bitmap; }

        [[nodiscard]]
        wxBitmap& GetBitmap() noexcept
            {
            return m_bitmap;
            }

      protected:
        wxBitmap m_bitmap;
        };

    /// @brief A thumbnail control, which includes previewing the full image and
    ///     (optional) drag 'n' drop support.
    class Thumbnail : public wxWindow
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param bmp The (full-size) bitmap to show in the thumbnail
                (control will downscale this).
            @param clickMode What to do when user clicks on the thumbnail.
            @param allowFileDrop Whether user can drag and drop an image from the
                file explorer into the control.
            @param id The control's window ID.
            @param pos The position for the control.
            @param size The default size for the control.
            @param style The window style.
            @param name The name of the control.*/
        Thumbnail(wxWindow* parent, const wxBitmap& bmp,
                  ClickMode clickMode = ClickMode::FullSizeViewable, bool allowFileDrop = false,
                  wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE,
                  const wxString& name = L"ThumbnailCtrl");
        /// @private
        Thumbnail() = delete;
        /// @private
        Thumbnail(const Thumbnail&) = delete;
        /// @private
        Thumbnail& operator=(const Thumbnail&) = delete;

        /** @brief Loads an image (from path) into the thumbnail.
            @param filePath The path to the image.
            @returns @c true if the image was successfully loaded.
            @note The image will be scaled down to the current size of the control
                (based on the image originally passed to the constructor).*/
        bool LoadImage(const wxString& filePath);
        /** @brief Sets the bitmap for the thumbnail.
            @param bmp The bitmap.
            @note This can be a full-size image, the thumbnail will scale it down for you.\n
                It will attempt to scale down to the current size of the control
                (based on the image originally passed to the constructor).*/
        void SetBitmap(const wxBitmap& bmp);

        /** @brief Sets the minimum size for the control.
            @details This is a hint, as the aspect ratio of the image will override this.
            @param minSize The minimum size to use.*/
        void SetMinSize(const wxSize& minSize) final;

        /** @brief Sets the opacity of the thumbnail when showing the image.
            @param opacity The opacity level (0-255).*/
        void SetOpacity(const uint8_t opacity)
            {
            m_opacity = opacity;
            Refresh();
            Update();
            }

        /// @returns The opacity of the thumbnail.
        [[nodiscard]]
        uint8_t GetOpacity() const noexcept
            {
            return m_opacity;
            }

        /// @returns The underlying image.
        [[nodiscard]]
        const Wisteria::GraphItems::Image& GetImage() const noexcept
            {
            return m_img;
            }

      private:
        void OnResize(wxSizeEvent& event);
        void OnClick([[maybe_unused]] wxMouseEvent& event);
        void OnPaint([[maybe_unused]] wxPaintEvent& event);

        Wisteria::GraphItems::Image m_img;
        ClickMode m_clickMode{ ClickMode::FullSizeViewable };
        uint8_t m_opacity{ wxALPHA_OPAQUE };
        wxSize m_baseSize{ 128, 128 };
        };

    /// @brief Drop file handler for thumbnail control.
    /// @private
    class DropThumbnailImageFile final : public wxFileDropTarget
        {
      public:
        explicit DropThumbnailImageFile(Thumbnail* pOwner = nullptr) noexcept : m_pOwner(pOwner) {}

        bool OnDropFiles([[maybe_unused]] wxCoord x, [[maybe_unused]] wxCoord y,
                         const wxArrayString& filenames) final;

      private:
        Thumbnail* m_pOwner{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_THUMBNAIL_H
