/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SVG_REPORT_PRINTOUT_H
#define WISTERIA_SVG_REPORT_PRINTOUT_H

#include "canvas.h"
#include <vector>

namespace Wisteria
    {
    /// @brief Exports a collection of canvases into a multipage SVG file.
    /// @details Each canvas is rendered into its own @c \<page\> element,
    ///     wrapped in a @c \<pageset\> inside a standard SVG document.
    ///     The page dimensions are derived from the paper size stored in
    ///     each canvas's printer settings.
    class SVGReportPrintout
        {
      public:
        /// @brief Constructor.
        /// @param canvases The canvases (pages) to export.
        /// @param filePath The file path to save the SVG to.
        SVGReportPrintout(const std::vector<Canvas*>& canvases, const wxString& filePath);

      private:
        /// @brief Retrieves the paper size (in DIPs) for the given canvas.
        /// @param canvas The canvas whose paper size to query.
        /// @returns The paper size as a wxSize.
        [[nodiscard]]
        static wxSize GetPaperSizeDIPs(const Canvas* canvas);

        /// @brief Strips the outer @c \<svg\> and @c \</svg\> tags from an SVG string,
        ///     returning just the body content.
        /// @param svgDoc The full SVG document string.
        /// @returns The SVG body without the surrounding svg element.
        [[nodiscard]]
        static wxString StripSvgTags(const wxString& svgDoc);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_SVG_REPORT_PRINTOUT_H
