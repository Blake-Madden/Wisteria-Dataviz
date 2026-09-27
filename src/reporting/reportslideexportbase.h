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

namespace Wisteria
    {
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
        /// @brief Renders one canvas to both an in-memory SVG document and in-memory PNG bytes,
        ///     laid out at renderSize (DIPs). The canvas is temporarily resized for the
        ///     render and restored afterward.
        static void RenderCanvas(Canvas* canvas, wxSize renderSize, wxString& svgOut,
                                 wxMemoryBuffer& pngOut);
        /// @returns @p color as an uppercase @c "RRGGBB" hex string (no leading @c '#').
        [[nodiscard]]
        static wxString ColorToHex(const wxColour& color);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_SLIDE_EXPORT_BASE_H
