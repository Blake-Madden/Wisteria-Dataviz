/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_SLIDE_EXPORT_BASE_H
#define WISTERIA_REPORT_SLIDE_EXPORT_BASE_H

#include "../base/canvas.h"
#include <wx/buffer.h>
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/utils.h>

namespace Wisteria
    {
    /// @brief Options common to slide-deck report exporters (e.g., @c .pptx, @c .odp).
    struct SlideExportOptionsBase
        {
        /// @brief The slide dimensions.
        enum class SlideSize
            {
            /// @brief Widescreen (16:9).
            Widescreen16x9,
            /// @brief Standard (4:3).
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

        /// @brief The maximum slide dimension, in inches.
        constexpr static double MAX_SLIDE_INCHES{ 56.0 };

        /// @brief The document title.
        wxString m_title;
        /// @brief The document author.
        wxString m_author{ wxGetUserName() };
        /// @brief The document subject.
        wxString m_subject;
        /// @brief The document keywords.
        wxString m_keywords;
        /// @brief The document publisher.
        wxString m_publisher;

        // title slide
        //------------

        /// @brief Whether to add a title slide before the report pages.
        bool m_includeTitleSlide{ true };
        /// @brief The lowercase key of a named color scheme for the title slide.
        wxString m_titleSlideTheme;

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
        /// @brief Whether slides advance automatically after @c m_advanceSeconds.
        bool m_advanceAutomatically{ false };
        /// @brief Seconds each slide is shown when @c m_advanceAutomatically is @c true.
        int m_advanceSeconds{ 5 };

        // notes
        //------

        /// @brief Whether to write chart accessibility descriptions as speaker notes.
        bool m_includeAccessibilityNotes{ true };
        };

    /// @brief Rendering and text-escaping helpers shared by slide-deck report
    ///     exporters (e.g., @c .pptx, @c .odp).
    class ReportSlideExportBase
        {
      protected:
        /// @brief One rendered page, ready to be written into a package.
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
        /// @brief Escapes text for a single-line XML attribute value. Collapses embedded
        ///     newlines/tabs down to single spaces before escaping.
        [[nodiscard]]
        static wxString EscapeXmlAttribute(const wxString& str);
        /// @brief Gathers a slide's speaker-notes text from the canvas titles and from every
        ///     fixed object's accessibility label (user override first, then the
        ///     auto-generated description). Paragraphs are separated by blank lines.
        [[nodiscard]]
        static wxString CollectAccessibilityText(Canvas* canvas);
        /// @brief Renders one canvas to an in-memory SVG document and, optionally, in-memory
        ///     PNG bytes, laid out at renderSize (DIPs). The canvas is temporarily resized for
        ///     the render and restored afterward.
        /// @param[out] pngOut Receives the rendered PNG bytes, or @c nullptr to skip the
        ///     raster render entirely.
        static void RenderCanvas(Canvas* canvas, wxSize renderSize, wxString& svgOut,
                                 wxMemoryBuffer* pngOut = nullptr);
        /// @returns @p color as an uppercase @c "RRGGBB" hex string (no leading @c '#').
        [[nodiscard]]
        static wxString ColorToHex(const wxColour& color);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_SLIDE_EXPORT_BASE_H
