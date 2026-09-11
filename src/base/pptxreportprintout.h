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

#include "canvas.h"
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
    struct PowerPointExportOptions
        {
        /// @brief The slide dimensions.
        enum class SlideSize
            {
            /// @brief 13.333in x 7.5in (PowerPoint "Widescreen").
            Widescreen16x9,
            /// @brief 10in x 7.5in (PowerPoint "Standard").
            Standard4x3,
            /// @brief A custom size, taken from @c m_customWidthInches / @c m_customHeightInches.
            Custom
            };

        /// @brief The deck-wide slide transition effect.
        enum class Transition
            {
            None,
            Fade,
            Push,
            Wipe,
            Split,
            Cut,
            Morph
            };

        /// @brief The transition playback speed.
        enum class TransitionSpeed
            {
            Slow,
            Medium,
            Fast
            };

        /// @brief English Metric Units per inch; slide dimensions in @c presentation.xml
        ///     are expressed in EMUs.
        constexpr static double EMU_PER_INCH{ 914400.0 };
        /// @brief PowerPoint's maximum slide dimension, in inches.
        constexpr static double MAX_SLIDE_INCHES{ 56.0 };

        /// @brief The document title.
        wxString m_title;
        /// @brief The document author.
        wxString m_author{ wxGetUserName() };
        /// @brief The document subject.
        wxString m_subject;
        /// @brief The document keywords.
        wxString m_keywords;

        // slide size
        //-----------

        /// @brief The slide size preset.
        SlideSize m_slideSize{ SlideSize::Widescreen16x9 };
        /// @brief Custom slide width in inches (used only when @c m_slideSize is @c Custom).
        double m_customWidthInches{ 13.333 };
        /// @brief Custom slide height in inches (used only when @c m_slideSize is @c Custom).
        double m_customHeightInches{ 7.5 };

        // transitions
        //------------

        /// @brief The deck-wide transition effect.
        Transition m_transition{ Transition::Fade };
        /// @brief The transition speed.
        TransitionSpeed m_transitionSpeed{ TransitionSpeed::Medium };

        // auto-advance / kiosk
        //---------------------

        /// @brief Whether a mouse click advances to the next slide.
        bool m_advanceOnClick{ true };
        /// @brief Whether slides advance automatically after @c m_advanceSeconds.
        bool m_advanceAutomatically{ false };
        /// @brief Seconds each slide is shown when @c m_advanceAutomatically is @c true.
        int m_advanceSeconds{ 5 };
        /// @brief Whether the slideshow loops continuously until Esc.
        bool m_loopContinuously{ false };

        // notes
        //------

        /// @brief Whether to write each page's auto-generated chart descriptions
        ///     into that slide's speaker-notes page.
        bool m_includeAccessibilityNotes{ true };

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
    ///     shows the crisp vector and other clients show the PNG. The package is an OPC
    ///     ZIP built directly with @c wxZipOutputStream.
    class ReportPowerPointExport
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

        // fixed OPC parts shared by every export
        static const std::wstring_view THEME_XML;
        static const std::wstring_view SLIDE_MASTER_XML;
        static const std::wstring_view SLIDE_LAYOUT_XML;
        static const std::wstring_view NOTES_MASTER_XML;

        /// @brief One rendered page, ready to be written into the package.
        struct RenderedPage
            {
            wxString m_svg;
            wxMemoryBuffer m_png;
            int m_pixelWidth{ 0 };
            int m_pixelHeight{ 0 };
            wxString m_notes;
            };

        /// @brief Escapes text for XML element content or attribute values and drops the
        ///     control characters that are illegal in XML 1.0.
        [[nodiscard]]
        static wxString EscapeXml(const wxString& str);
        [[nodiscard]]
        static wxString BuildRelationshipsXml(
            const std::vector<std::tuple<wxString, wxString, wxString>>& relationships);
        /// @brief Gathers a slide's speaker-notes text from the canvas titles and from every
        ///     fixed object's accessibility label (user override first, then the
        ///     auto-generated description). Paragraphs are separated by blank lines.
        [[nodiscard]]
        static wxString CollectAccessibilityText(Canvas* canvas);
        /// @brief Renders one canvas to both an in-memory SVG document and in-memory PNG bytes,
        ///     laid out at renderSize (DIPs). The canvas is temporarily resized for the
        ///     render and restored afterward.
        static void RenderCanvas(Canvas* canvas, wxSize renderSize, wxString& svgOut,
                                 wxMemoryBuffer& pngOut);
        /// @brief The <p:transition> element (or MCE AlternateContent for Morph) for a slide,
        ///      or an empty string when there is nothing to emit.
        [[nodiscard]]
        static wxString BuildTransitionXml(const PowerPointExportOptions& options);
        /// @brief Positions the page picture within the slide.
        //  @details Fills the slide when the aspect matches; otherwise,  scales to
        //      contain and centers (letterbox on the slide bg).
        [[nodiscard]]
        static wxString BuildPicturePlacementXml(long long slideCx, long long slideCy,
                                                 int imageWidth, int imageHeight);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_PPTX_EXPORT_H
