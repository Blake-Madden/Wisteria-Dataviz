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
    /// @brief Options for SVG report export.
    struct SVGReportOptions
        {
        /// @brief Constructor.
        /// @param filePath The file path to save the SVG to.
        explicit SVGReportOptions(const wxString& filePath) : m_filePath(filePath) {}

        /// @brief The file path to save the SVG to.
        wxString m_filePath;
        /// @brief Whether to include smooth transitions (sliding pages).
        bool m_includeTransitions{ true };
        /// @brief Whether to include interactive highlighting on hover.
        bool m_includeHighlighting{ true };
        /// @brief Whether to include a floating layout toggle (stacked vs duplex) and
        ///     page-gap spinner.
        bool m_includeLayoutOptions{ true };
        /// @brief Whether to include a floating dark-mode toggle.
        bool m_includeDarkModeToggle{ true };
        /// @brief Whether to include slideshow navigation (arrow keys + prev/next buttons).
        bool m_includeSlideshow{ true };
        /// @brief Whether to include a subtle page shadow.
        bool m_includePageShadow{ true };
        /// @brief The background color for the overlay buttons and effects.
        wxColour m_themeColor{ 103, 58, 183 };
        /// @brief The horizontal gap (in pixels) between rows of pages.
        int m_horizontalGap{ 25 };
        /// @brief Uniform page size (in DIPs). If default, uses per-canvas paper sizes.
        wxSize m_pageSize{ wxDefaultSize };

        [[nodiscard]]
        bool HasInteractiveFeatures() const noexcept
            {
            return m_includeTransitions || m_includeHighlighting || m_includeLayoutOptions ||
                   m_includeDarkModeToggle || m_includeSlideshow || m_includePageShadow;
            }

        /// @returns @c true if any floating UI overlay (buttons, progress bar) is enabled.
        [[nodiscard]]
        bool HasUILayer() const noexcept
            {
            return m_includeLayoutOptions || m_includeDarkModeToggle || m_includeSlideshow;
            }

        /// @brief Enables/disables smooth transitions.
        SVGReportOptions& Transitions(bool include)
            {
            m_includeTransitions = include;
            return *this;
            }

        /// @brief Enables/disables interactive highlighting.
        SVGReportOptions& Highlighting(bool include)
            {
            m_includeHighlighting = include;
            return *this;
            }

        /// @brief Enables/disables the layout options.
        SVGReportOptions& LayoutOptions(bool include)
            {
            m_includeLayoutOptions = include;
            return *this;
            }

        /// @brief Enables/disables the dark-mode toggle.
        SVGReportOptions& DarkModeToggle(bool include)
            {
            m_includeDarkModeToggle = include;
            return *this;
            }

        /// @brief Enables/disables slideshow navigation.
        SVGReportOptions& Slideshow(bool include)
            {
            m_includeSlideshow = include;
            return *this;
            }

        /// @brief Enables/disables the page shadow.
        SVGReportOptions& PageShadow(bool include)
            {
            m_includePageShadow = include;
            return *this;
            }

        /// @brief Sets the theme color.
        SVGReportOptions& ThemeColor(const wxColour& color)
            {
            m_themeColor = color;
            return *this;
            }

        /// @brief Sets the horizontal gap between rows.
        SVGReportOptions& HorizontalGap(int gap)
            {
            m_horizontalGap = gap;
            return *this;
            }

        /// @brief Sets a uniform page size.
        SVGReportOptions& PageSize(const wxSize& size)
            {
            m_pageSize = size;
            return *this;
            }
        };

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
        /// @param options Export options (interactivity, sizing, file path, etc.).
        SVGReportPrintout(const std::vector<Canvas*>& canvases, SVGReportOptions options);

        /// @brief Retrieves the paper size (in DIPs) for the given canvas.
        /// @param canvas The canvas whose paper size to query.
        /// @returns The paper size as a wxSize.
        [[nodiscard]]
        static wxSize GetPaperSizeDIPs(const Canvas* canvas);

        /// @brief Generates CSS dark-mode fill replacement rules for very light colors.
        /// @param svgContent The SVG content to analyze.
        /// @returns A string of CSS rules for dark-mode mapping.
        [[nodiscard]]
        static wxString GenerateDarkModeFillReplacements(std::wstring_view svgContent);

      private:
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
