/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_IMAGE_DIALOG_H
#define INSERT_IMAGE_DIALOG_H

#include "../../base/image.h"
#include "insertitemdlg.h"
#include <wx/editlbox.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting or editing an Image on a canvas cell.
        @details Extends InsertItemDlg with an "Image" page containing:
            - One or more image file paths (via editable list box with
              built-in New/Delete/Up/Down buttons).
            - Stitch direction when multiple images are provided
              (horizontal or vertical).
            - Size (width / height, aspect-ratio preserved).
            - Resize method (downscale-or-upscale, downscale-only,
              upscale-only, no-resize).
            - Image effect (none, grayscale, sepia, oil painting,
              frosted glass, blur horizontal, blur vertical).

        Can also be used standalone to edit an existing Image's settings
        by calling LoadFromImage() before showing the dialog and
        ApplyToImage() after it returns wxID_OK.*/
    class InsertImageDlg final : public InsertItemDlg
        {
      public:
        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder (may be @c nullptr).
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.
            @param includePageOptions Whether to show the "Page Options"
                section.*/
        InsertImageDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                       const wxString& caption = _(L"Insert Image"), wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                       EditMode editMode = EditMode::Insert, bool includePageOptions = true);

        /// @private
        InsertImageDlg(const InsertImageDlg&) = delete;
        /// @private
        InsertImageDlg& operator=(const InsertImageDlg&) = delete;

        /// @returns The image file paths.
        [[nodiscard]]
        wxArrayString GetImagePaths() const;

        /// @returns @c true if the user opted to override the default size.
        [[nodiscard]]
        bool IsCustomSizeEnabled() const noexcept
            {
            return m_customSize;
            }

        /// @returns The requested width (only meaningful when
        ///     IsCustomSizeEnabled() is @c true).
        [[nodiscard]]
        int GetImageWidth() const;

        /// @returns The requested height (only meaningful when
        ///     IsCustomSizeEnabled() is @c true).
        [[nodiscard]]
        int GetImageHeight() const;

        /// @returns The selected resize method.
        [[nodiscard]]
        ResizeMethod GetResizeMethod() const noexcept
            {
            return static_cast<ResizeMethod>(m_resizeMethod);
            }

        /// @returns The selected image effect.
        [[nodiscard]]
        ImageEffect GetImageEffect() const noexcept
            {
            return static_cast<ImageEffect>(m_imageEffect);
            }

        /// @returns The stitch direction string
        ///     ("horizontal" or "vertical").
        [[nodiscard]]
        wxString GetStitchDirection() const
            {
            return (m_stitchDirection == 1) ? L"vertical" : L"horizontal";
            }

        /// @brief Populates controls from an existing image.
        /// @param image The image to read settings from.
        /// @param canvas The canvas the image belongs to.
        void LoadFromImage(const Wisteria::GraphItems::Image& image, Canvas* canvas);

        /// @brief Applies the dialog settings to an image.
        /// @param image The image to update.
        void ApplyToImage(Wisteria::GraphItems::Image& image) const;

      private:
        void CreateControls() final;
        bool Validate() final;
        void OnEnableCustomSize(bool enable);

        // starts at +3 to avoid collision with InsertItemDlg (+1)
        // and InsertLabelDlg (+2)
        constexpr static wxWindowID ID_IMAGE_SECTION{ wxID_HIGHEST + 3 };

        bool m_includePageOptions{ true };

        // controls
        wxEditableListBox* m_pathListBox{ nullptr };
        wxChoice* m_stitchChoice{ nullptr };
        wxSpinCtrl* m_widthSpin{ nullptr };
        wxSpinCtrl* m_heightSpin{ nullptr };

        // DDX data members
        bool m_customSize{ false };
        int m_resizeMethod{ 0 };
        int m_imageEffect{ 0 };
        int m_stitchDirection{ 0 };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_IMAGE_DIALOG_H
