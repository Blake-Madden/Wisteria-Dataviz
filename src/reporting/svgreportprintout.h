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

#include "../base/canvas.h"
#include <utility>
#include <vector>

namespace Wisteria
    {
    /// @brief Options for SVG report export.
    struct SVGReportOptions
        {
        /// @brief How the pages in the SVG are laid out.
        enum class PageLayout
            {
            /// @brief Pages are in a single column.
            Single,
            /// @brief Pages are side-by-side (2x2 grid).
            Duplex,
            /// @brief Pages are stacked with southeast offset.
            Stacked
            };

        /// @brief Constructor.
        /// @param filePath The file path to save the SVG to.
        explicit SVGReportOptions(wxString filePath) : m_filePath(std::move(filePath)) {}

        /// @brief The file path to save the SVG to.
        wxString m_filePath;
        /// @brief Whether to include smooth transitions (sliding pages).
        bool m_includeTransitions{ true };
        /// @brief Whether to include interactive highlighting on hover.
        bool m_includeHighlighting{ true };
        /// @brief Whether to include a floating layout toggle (stacked vs duplex) and
        ///     page-gap spinner.
        bool m_includeLayoutOptions{ true };
        /// @brief How the pages in the SVG are laid out.
        PageLayout m_layout{ PageLayout::Duplex };
        /// @brief Whether to include a floating dark-mode toggle.
        bool m_includeDarkModeToggle{ true };
        /// @brief Whether to include slideshow navigation (arrow keys + prev/next buttons).
        bool m_includeSlideshow{ true };
        /// @brief Whether to include a subtle page shadow.
        bool m_includePageShadow{ true };
        /// @brief Whether to include layer filter checkboxes (when pages have layers).
        bool m_includeLayerControls{ true };
        /// @brief The background color for the overlay buttons and effects.
        wxColour m_themeColor{ 103, 58, 183 };
        /// @brief Uniform page size (in DIPs). If default, uses per-canvas paper sizes.
        wxSize m_pageSize{ wxDefaultSize };
        /// @brief Whether to use the global print settings (paper size and orientation)
        ///     for the SVG dimensions.
        bool m_useGlobalPrintSettings{ true };
        /// @brief The paper size to use when @c m_useGlobalPrintSettings is @c true.
        wxPaperSize m_paperId{ wxPAPER_LETTER };
        /// @brief The paper orientation to use when @c m_useGlobalPrintSettings is @c true.
        wxPrintOrientation m_paperOrientation{ wxPORTRAIT };

        /// @brief Whether to include any interactive features.
        /// @returns @c true if any interactive features are enabled.
        [[nodiscard]]
        bool HasInteractiveFeatures() const noexcept
            {
            return m_includeTransitions || m_includeHighlighting || m_includeLayoutOptions ||
                   m_includeDarkModeToggle || m_includeSlideshow || m_includePageShadow;
            }

        /// @returns @c true if any floating UI overlay (buttons, progress bar) is enabled.
        /// @note This does not account for layer controls, which also depend on whether any
        ///     page has a layer. Callers combine this with HasLayerControls().
        [[nodiscard]]
        bool HasUILayer() const noexcept
            {
            return m_includeLayoutOptions || m_includeDarkModeToggle;
            }

        /// @returns @c true if layer controls should be shown (option enabled and layers exist).
        /// @param layers The collection of layers to check for emptiness.
        template<typename Container>
        [[nodiscard]]
        bool HasLayerControls(const Container& layers) const noexcept
            {
            return m_includeLayerControls && !layers.empty();
            }

        /// @brief Enables/disables smooth transitions.
        /// @param include @c true to include smooth transitions.
        /// @returns A reference to this object.
        SVGReportOptions& Transitions(bool include)
            {
            m_includeTransitions = include;
            return *this;
            }

        /// @brief How the pages are organized.
        /// @param layout The page layout.
        /// @returns A reference to this object.
        SVGReportOptions& Layout(PageLayout layout)
            {
            m_layout = layout;
            return *this;
            }

        /// @brief Enables/disables interactive highlighting.
        /// @param include @c true to include interactive highlighting.
        /// @returns A reference to this object.
        SVGReportOptions& Highlighting(bool include)
            {
            m_includeHighlighting = include;
            return *this;
            }

        /// @brief Enables/disables the layout options.
        /// @param include @c true to include layout options.
        /// @returns A reference to this object.
        SVGReportOptions& LayoutOptions(bool include)
            {
            m_includeLayoutOptions = include;
            return *this;
            }

        /// @brief Enables/disables the dark-mode toggle.
        /// @param include @c true to include the dark-mode toggle.
        /// @returns A reference to this object.
        SVGReportOptions& DarkModeToggle(bool include)
            {
            m_includeDarkModeToggle = include;
            return *this;
            }

        /// @brief Enables/disables slideshow navigation.
        /// @param include @c true to include slideshow navigation.
        /// @returns A reference to this object.
        SVGReportOptions& Slideshow(bool include)
            {
            m_includeSlideshow = include;
            return *this;
            }

        /// @brief Enables/disables the page shadow.
        /// @param include @c true to include a page shadow.
        /// @returns A reference to this object.
        SVGReportOptions& PageShadow(bool include)
            {
            m_includePageShadow = include;
            return *this;
            }

        /// @brief Enables/disables layer filter checkboxes.
        /// @param include @c true to include layer controls.
        /// @returns A reference to this object.
        SVGReportOptions& LayerControls(bool include)
            {
            m_includeLayerControls = include;
            return *this;
            }

        /// @brief Sets the theme color.
        /// @param color The theme color.
        /// @returns A reference to this object.
        SVGReportOptions& ThemeColor(const wxColour& color)
            {
            m_themeColor = color;
            return *this;
            }

        /// @brief Sets a uniform page size.
        /// @param size The page size.
        /// @returns A reference to this object.
        SVGReportOptions& PageSize(const wxSize& size)
            {
            m_pageSize = size;
            return *this;
            }

        /// @brief Sets whether to use global print settings for the SVG dimensions.
        /// @param use @c true to use global print settings.
        /// @returns A reference to this object.
        SVGReportOptions& UseGlobalPrintSettings(bool use)
            {
            m_useGlobalPrintSettings = use;
            return *this;
            }

        /// @brief Sets the paper size for export.
        /// @param paperId The paper size.
        /// @returns A reference to this object.
        SVGReportOptions& PaperId(wxPaperSize paperId)
            {
            m_paperId = paperId;
            return *this;
            }

        /// @brief Sets the paper orientation for export.
        /// @param orientation The paper orientation.
        /// @returns A reference to this object.
        SVGReportOptions& PaperOrientation(wxPrintOrientation orientation)
            {
            m_paperOrientation = orientation;
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
        /// @brief The horizontal gap (in pixels) between rows of pages.
        constexpr static int PAGE_GAP{ 25 };

        /// @brief Constructor.
        /// @param canvases The canvases (pages) to export.
        /// @param options Export options (interactivity, sizing, file path, etc.).
        SVGReportPrintout(const std::vector<Canvas*>& canvases, SVGReportOptions options);

        /// @brief Retrieves the paper size (in DIPs) for the given canvas.
        /// @param canvas The canvas whose paper size to query.
        /// @returns The paper size as a wxSize.
        [[nodiscard]]
        static wxSize GetPaperSizeDIPs(const Canvas* canvas);

        /// @brief Retrieves the paper size (in DIPs) for the given paper type and orientation.
        /// @param paperId The paper type.
        /// @param orientation The paper orientation.
        /// @returns The paper size as a wxSize.
        [[nodiscard]]
        static wxSize GetPaperSizeDIPs(wxPaperSize paperId, wxPrintOrientation orientation);

        /// @brief Generates CSS dark-mode fill replacement rules for very light colors.
        /// @param svgContent The SVG content to analyze.
        /// @returns A string of CSS rules for dark-mode mapping.
        [[nodiscard]]
        static wxString GenerateDarkModeFillReplacements(std::wstring_view svgContent);

        /// @brief Renders a canvas as an SVG fragment, without the outer @c \<svg\> element.
        /// @param canvas The canvas to render.
        /// @param layoutSize The size (in DIPs) to render at.
        /// @returns The SVG body of the canvas.
        [[nodiscard]]
        static wxString RenderCanvasToSvg(Canvas* canvas, const wxSize& layoutSize);

        /// @brief Strips the outer @c \<svg\> and @c \</svg\> tags from an SVG string,
        ///     returning just the body content.
        /// @param svgDoc The full SVG document string.
        /// @returns The SVG body without the surrounding svg element.
        [[nodiscard]]
        static wxString StripSvgTags(const wxString& svgDoc);

        /// @brief Escapes a string for use inside a double-quoted XML attribute value.
        /// @param str The raw string.
        /// @returns The escaped string.
        [[nodiscard]]
        static wxString EscapeXmlAttr(const wxString& str);

        /// @brief Escapes a string for use as XML element text content.
        /// @param str The raw string.
        /// @returns The escaped string.
        [[nodiscard]]
        static wxString EscapeXmlText(const wxString& str);

        /// @brief Escapes a string for use as a single-quoted JavaScript string literal.
        /// @param str The raw string.
        /// @returns The escaped string.
        [[nodiscard]]
        static wxString EscapeJsString(const wxString& str);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_SVG_REPORT_PRINTOUT_H
