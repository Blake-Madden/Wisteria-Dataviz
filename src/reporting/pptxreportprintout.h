/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_PPTX_EXPORT_H
#define WISTERIA_REPORT_PPTX_EXPORT_H

#include "../base/canvas.h"
#include "reportslideexportbase.h"
#include <string_view>
#include <tuple>
#include <vector>
#include <wx/buffer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

namespace Wisteria
    {
    /// @brief Options for PowerPoint (@c .pptx) report export.
    /// @details Plain aggregate, like @c PdfExportOptions. Holds document metadata,
    ///     the slide size, the deck-wide transition, auto-advance/loop behavior, and
    ///     whether chart accessibility descriptions are written as speaker notes.
    struct PowerPointExportOptions : public SlideExportOptionsBase
        {
        /// @brief English Metric Units per inch. Slide dimensions in @c presentation.xml
        ///     are expressed in EMUs.
        constexpr static double EMU_PER_INCH{ 914400.0 };
        /// @brief The height, in EMUs, shared by the Standard (4:3) and Widescreen (16:9)
        ///     slide sizes (7.5in).
        constexpr static int SLIDE_HEIGHT_EMU{ 6858000 };
        /// @brief The Standard (4:3) slide width, in EMUs (10in).
        constexpr static int SLIDE_WIDTH_4X3_EMU{ 9144000 };
        /// @brief The Widescreen (16:9) slide width, in EMUs (13.333in).
        constexpr static int SLIDE_WIDTH_16X9_EMU{ 12192000 };

        // auto-advance / kiosk (not modeled by the ODP exporter)
        //---------------------

        /// @brief Whether a mouse click advances to the next slide.
        bool m_advanceOnClick{ true };
        /// @brief Whether the slideshow loops continuously until Esc.
        bool m_loopContinuously{ false };

        /// @returns The slide size in English Metric Units (@c EMU_PER_INCH per inch).
        [[nodiscard]]
        wxSize GetSlideSizeEMU() const;

        /// @returns The slide size in DIPs (96 per inch), used as the render target size.
        [[nodiscard]]
        wxSize GetSlideSizeDIPs() const;
        };

    /// @brief Exports a collection of canvases as slides into a single @c .pptx file.
    /// @details Each canvas becomes one slide, rendered to fill the slide. The slide
    ///     picture references a rasterized PNG (the OOXML picture blip) with the
    ///     vector SVG attached through the DrawingML SVG extension, so PowerPoint 2016+
    ///     shows the crisp vector and other clients show the PNG.
    class ReportPowerPointExport : public ReportSlideExportBase
        {
      public:
        /** @brief Constructor. Exports all canvases as slides to a @c .pptx file immediately.
            @param canvases The canvases (pages) to export.
            @param filePath The output @c .pptx file path.
            @param options The PowerPoint export options.*/
        ReportPowerPointExport(const std::vector<Canvas*>& canvases, const wxString& filePath,
                               const PowerPointExportOptions& options = PowerPointExportOptions{});

      private:
        // OOXML relationship type URIs
        constexpr static std::wstring_view REL_OFFICE_DOCUMENT{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
        };
        constexpr static std::wstring_view REL_CORE_PROPS{
            L"http://schemas.openxmlformats.org/package/2006/relationships/metadata/"
            L"core-properties"
        };
        constexpr static std::wstring_view REL_EXTENDED_PROPS{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
            L"extended-properties"
        };
        constexpr static std::wstring_view REL_SLIDE_MASTER{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster"
        };
        constexpr static std::wstring_view REL_SLIDE_LAYOUT{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout"
        };
        constexpr static std::wstring_view REL_SLIDE{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide"
        };
        constexpr static std::wstring_view REL_NOTES_MASTER{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/notesMaster"
        };
        constexpr static std::wstring_view REL_NOTES_SLIDE{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/notesSlide"
        };
        constexpr static std::wstring_view REL_THEME{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme"
        };
        constexpr static std::wstring_view REL_PRES_PROPS{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps"
        };
        constexpr static std::wstring_view REL_IMAGE{
            L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"
        };

        /// @brief The speaker-notes page width, in EMUs (7.5in, portrait).
        constexpr static long long NOTES_PAGE_WIDTH_EMU{ 6858000 };
        /// @brief The speaker-notes page height, in EMUs (10in, portrait).
        constexpr static long long NOTES_PAGE_HEIGHT_EMU{ 9144000 };
        /// @brief Milliseconds per second, for the slide @c advTm (auto-advance) attribute.
        constexpr static int MILLISECONDS_PER_SECOND{ 1000 };
        /// @brief OOXML alpha units per percent. An @c \<a:alpha\> value is in thousandths
        ///     of a percent, so 100000 is fully opaque.
        constexpr static int ALPHA_UNITS_PER_PERCENT{ 1000 };
        /// @brief The maximum length, in characters, of a slide picture's alt text.
        constexpr static size_t MAX_ALT_TEXT_LENGTH{ 2000 };

        // fixed OPC parts shared by every export
        static const std::wstring_view THEME_XML;
        static const std::wstring_view SLIDE_MASTER_XML;
        static const std::wstring_view SLIDE_LAYOUT_XML;
        static const std::wstring_view NOTES_MASTER_XML;

        [[nodiscard]]
        static wxString BuildRelationshipsXml(
            const std::vector<std::tuple<wxString, wxString, wxString>>& relationships);
        /// @returns The @c \<p:transition\> element (or MCE @c AlternateContent for Morph)
        ///     for a slide, or an empty string when there is nothing to emit.
        [[nodiscard]]
        static wxString BuildTransitionXml(const PowerPointExportOptions& options);
        /// @brief Positions the page picture within the slide.
        /// @details Fills the slide when the aspect ratios match. Otherwise, scales the
        ///     picture to fit and centers it, letterboxed on the slide background.
        [[nodiscard]]
        static wxString BuildPicturePlacementXml(long long slideCx, long long slideCy,
                                                 int imageWidth, int imageHeight);
        /// @brief The title slide's @c \<p:sld\> content: the deck title, centered, with
        ///     the author beneath it as a subtitle when set, and the publisher, when
        ///     set, at the bottom of the slide. When @c options.m_titleSlideTheme names a
        ///     known color scheme, the slide also gets a gradient background, a left
        ///     accent bar, a decorative accent circle, and themed text colors.
        ///     @p transitionXml gives the title slide the same transition and
        ///     advance timing as the report slides.
        [[nodiscard]]
        static wxString BuildTitleSlideXml(const PowerPointExportOptions& options,
                                           long long slideCx, long long slideCy,
                                           const wxString& transitionXml);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_PPTX_EXPORT_H
