/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef IMAGEEXPORT_DLG_H
#define IMAGEEXPORT_DLG_H

#include "../../math/mathematics.h"
#include "../../util/donttranslate.h"
#include "../controls/thumbnail.h"
#include <utility>
#include <wx/bitmap.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/fontdlg.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/spinctrl.h>
#include <wx/tooltip.h>
#include <wx/valgen.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Tiff compression methods.
    /// @details These match the \#defines in libtiff and are included
    ///     as an enumeration here so that @c tiff.h doesn't need to be included.
    enum class TiffCompression
        {
        CompressionNone = 1,       /*!< COMPRESSION_NONE*/
        CompressionLZW = 5,        /*!< COMPRESSION_LZW*/
        CompressionJPEG = 7,       /*!< COMPRESSION_JPEG*/
        CompressionDeflate = 32946 /*!< COMPRESSION_DEFLATE*/
        };

    /// @brief Options for exporting an image.
    struct ImageExportOptions
        {
        /// @brief Color modes to save an image with.
        enum class ColorMode
            {
            /// @brief Color image.
            RGB,
            /// @brief Shades of gray (i.e., B & W).
            Grayscale,
            /// @private
            Greyscale = Grayscale
            };
        /// @private
        ImageExportOptions() = default;
        /// @brief The color mode. Really a ColorMode,
        ///     but must be int to be compatible with a validator.
        int m_mode{ static_cast<decltype(m_mode)>(ColorMode::RGB) };
        /// @brief The Tiff compression method (if saving as Tiff).
        TiffCompression m_tiffCompression{ TiffCompression::CompressionNone };
        /// @brief The dimensions of the exported image (in DIPs).
        wxSize m_imageSize{ wxSize(700, 500) };
        };

    /// @brief Options dialog for saving an image.
    /// @details Includes options for color/B&W, tiff compression, etc.\n
    ///     Canvas save events use this dialog, so normally client code should not
    ///     need to use this interface.
    class ImageExportDlg final : public wxDialog
        {
      public:
        /** @brief Default constructor.
            @param parent The parent window.
            @param bitmapType The image format to export as.
            @param previewImg The preview image to show on the dialog.
            @param options The image export options.
            @param id The window ID.
            @param caption The title of the export dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        ImageExportDlg(wxWindow* parent, const wxBitmapType bitmapType, wxBitmap previewImg,
                       const ImageExportOptions& options, wxWindowID id = wxID_ANY,
                       const wxString& caption = _(L"Image Export Options"),
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       const long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN)
            : m_options(options), m_originalBitmap(std::move(previewImg))
            {
            Create(parent, bitmapType, id, caption, pos, size, style);
            }

        /// @brief Two-step constructor.
        ImageExportDlg() = default;
        /// @private
        ImageExportDlg(const ImageExportDlg&) = delete;
        /// @private
        ImageExportDlg& operator=(const ImageExportDlg&) = delete;
        /** @brief Constructs the dialog.
            @details This is part of a 2-step construction and should be
                called after the empty constructor.
            @param parent The parent window.
            @param bitmapType The image format to export as.
            @param id The window ID.
            @param caption The title of the export dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).
            @returns @c true if creation was successful.*/
        bool Create(wxWindow* parent, wxBitmapType bitmapType, wxWindowID id = wxID_ANY,
                    const wxString& caption = _(L"Image Export Options"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN);

        /// @returns The options selected by the user.
        [[nodiscard]]
        ImageExportOptions GetOptions() const noexcept
            {
            return m_options;
            }

        /** @brief Sets the help topic for the dialog.
            @param helpProjectDirectory The folder/base URL where the topics are stored.
            @param topicPath The path (after @c helpProjectDirectory) to the topic.*/
        void SetHelpTopic(const wxString& helpProjectDirectory, const wxString& topicPath)
            {
            m_helpProjectFolder = helpProjectDirectory;
            m_helpTopic = topicPath;
            }

      private:
        void CreateControls(wxBitmapType bitmapType);

        void OnOK([[maybe_unused]] wxCommandEvent& event);
        void OnOptionsChanged([[maybe_unused]] wxCommandEvent& event);
        void OnSizeChanged(const wxSpinEvent& event);

        void OnHelpClicked([[maybe_unused]] wxCommandEvent& event)
            {
            if (!m_helpTopic.empty())
                {
                wxLaunchDefaultBrowser(wxFileName::FileNameToURL(
                    m_helpProjectFolder + wxFileName::GetPathSeparator() + m_helpTopic));
                }
            }

        void OnContextHelp([[maybe_unused]] wxHelpEvent& event)
            {
            wxCommandEvent cmd;
            OnHelpClicked(cmd);
            }

        enum ControlIDs
            {
            COLOR_MODE_COMBO_ID = wxID_HIGHEST,
            IMAGE_WIDTH_ID,
            IMAGE_HEIGHT_ID
            };

        ImageExportOptions m_options;

        wxBitmap m_originalBitmap{ wxNullBitmap };

        wxComboBox* m_tiffCompressionCombo{ nullptr };
        Thumbnail* m_previewThumbnail{ nullptr };

        wxString m_helpProjectFolder;
        wxString m_helpTopic;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // IMAGEEXPORT_DLG_H
