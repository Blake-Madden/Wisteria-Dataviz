/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_HTML_DASHBOARD_PRINTOUT_H
#define WISTERIA_HTML_DASHBOARD_PRINTOUT_H

#include "../base/canvas.h"
#include <utility>
#include <vector>

namespace Wisteria
    {
    /// @brief Options for HTML dashboard export.
    struct HtmlDashboardOptions
        {
        /// @brief The initial light/dark mode of the dashboard.
        enum class ColorMode
            {
            /// @brief Follows the operating system.
            Auto,
            /// @brief Always light.
            Light,
            /// @brief Always dark.
            Dark
            };

        /// @brief How the pages are initially presented.
        enum class DashboardView
            {
            /// @brief Full-height pages in a scrolling column.
            Storyline,
            /// @brief Small page cards across the top, with the selected page shown below.
            Gallery
            };

        /// @brief Constructor.
        /// @param filePath The file path to save the HTML to.
        explicit HtmlDashboardOptions(wxString filePath) : m_filePath(std::move(filePath)) {}

        /// @brief The file path to save the HTML to.
        wxString m_filePath;
        /// @brief The dashboard title. If empty, the first canvas label is used.
        wxString m_title;
        /// @brief Path to an optional logo shown in the title bar.
        wxString m_logoPath;
        /// @brief The CSS to inline (core stylesheet followed by the theme).
        wxString m_css;
        /// @brief The initial light/dark mode.
        ColorMode m_colorMode{ ColorMode::Auto };
        /// @brief How the pages are initially presented.
        DashboardView m_view{ DashboardView::Gallery };
        /// @brief Whether to include the Auto/Light/Dark toggle.
        bool m_includeColorModeToggle{ true };
        /// @brief Whether large numbers count up when a page is revealed.
        bool m_countUpNumbers{ true };
        /// @brief The size (in DIPs) that every page is rendered at.
        wxSize m_pageSize{ 1280, 720 };
        /// @brief The name of the theme that @c m_css was built from.
        /// @note This is only stored by the caller. The exporter uses @c m_css.
        wxString m_theme;
        /// @brief Page titles, in the same order as the canvases.
        /// @details An empty or missing entry falls back to the canvas's label.
        std::vector<wxString> m_pageTitles;

        /// @param view The view.
        /// @returns The view as a string (for saving).
        [[nodiscard]]
        static wxString ViewToString(const DashboardView view)
            {
            return (view == DashboardView::Gallery) ? L"gallery" : L"story";
            }

        /// @param str The string to parse.
        /// @param fallback The view to return if @p str is not recognized.
        /// @returns The view for the string.
        [[nodiscard]]
        static DashboardView ParseView(const wxString& str, const DashboardView fallback)
            {
            if (str == L"gallery")
                {
                return DashboardView::Gallery;
                }
            if (str == L"story")
                {
                return DashboardView::Storyline;
                }
            return fallback;
            }

        /// @param mode The color mode.
        /// @returns The color mode as a string (for saving).
        [[nodiscard]]
        static wxString ColorModeToString(const ColorMode mode)
            {
            if (mode == ColorMode::Light)
                {
                return L"light";
                }
            return (mode == ColorMode::Dark) ? L"dark" : L"auto";
            }

        /// @param str The string to parse.
        /// @param fallback The color mode to return if @p str is not recognized.
        /// @returns The color mode for the string.
        [[nodiscard]]
        static ColorMode ParseColorMode(const wxString& str, const ColorMode fallback)
            {
            if (str == L"auto")
                {
                return ColorMode::Auto;
                }
            if (str == L"light")
                {
                return ColorMode::Light;
                }
            return (str == L"dark") ? ColorMode::Dark : fallback;
            }

        /// @brief Sets the dashboard title.
        /// @param title The title.
        /// @returns A reference to this object.
        HtmlDashboardOptions& Title(const wxString& title)
            {
            m_title = title;
            return *this;
            }

        /// @brief Sets the logo shown at the left of the title bar.
        /// @param logoPath The path to the image file. Empty for no logo.
        /// @returns A reference to this object.
        HtmlDashboardOptions& Logo(const wxString& logoPath)
            {
            m_logoPath = logoPath;
            return *this;
            }

        /// @brief Sets the CSS to inline.
        /// @param css The core stylesheet followed by the theme.
        /// @returns A reference to this object.
        HtmlDashboardOptions& Css(const wxString& css)
            {
            m_css = css;
            return *this;
            }

        /// @brief Sets the initial light/dark mode.
        /// @param mode The mode.
        /// @returns A reference to this object.
        HtmlDashboardOptions& InitialColorMode(const ColorMode mode) noexcept
            {
            m_colorMode = mode;
            return *this;
            }

        /// @brief Sets how the pages are initially presented.
        /// @param view The view.
        /// @returns A reference to this object.
        HtmlDashboardOptions& InitialView(const DashboardView view) noexcept
            {
            m_view = view;
            return *this;
            }

        /// @brief Enables/disables the Auto/Light/Dark toggle.
        /// @param include @c true to include the toggle.
        /// @returns A reference to this object.
        HtmlDashboardOptions& ColorModeToggle(const bool include) noexcept
            {
            m_includeColorModeToggle = include;
            return *this;
            }

        /// @brief Enables/disables counting up large numbers when a page is revealed.
        /// @param countUp @c true to count numbers up.
        /// @returns A reference to this object.
        HtmlDashboardOptions& CountUpNumbers(const bool countUp) noexcept
            {
            m_countUpNumbers = countUp;
            return *this;
            }

        /// @brief Sets the name of the theme that the CSS was built from.
        /// @param theme The theme name.
        /// @returns A reference to this object.
        HtmlDashboardOptions& Theme(const wxString& theme)
            {
            m_theme = theme;
            return *this;
            }

        /// @brief Sets the page titles.
        /// @param titles The titles, in the same order as the canvases.
        /// @returns A reference to this object.
        HtmlDashboardOptions& PageTitles(std::vector<wxString> titles)
            {
            m_pageTitles = std::move(titles);
            return *this;
            }

        /// @brief Sets the size that every page is rendered at.
        /// @param size The page size (in DIPs).
        /// @returns A reference to this object.
        HtmlDashboardOptions& PageSize(const wxSize& size)
            {
            m_pageSize = size;
            return *this;
            }
        };

    /// @brief Exports a collection of canvases into a single self-contained HTML5 file.
    class HtmlDashboardPrintout
        {
      public:
        /// @brief Constructor.
        /// @param canvases The canvases (pages) to export.
        /// @param options Export options.
        HtmlDashboardPrintout(const std::vector<Canvas*>& canvases, HtmlDashboardOptions options);

        /// @returns The script managing color mode, view, and layer state.
        [[nodiscard]]
        static wxString GetDashboardScriptState();
        /// @returns The script building the page rail and gallery cards.
        [[nodiscard]]
        static wxString GetDashboardScriptPages();
        /// @returns The script tagging shapes so their painted color survives dark mode.
        [[nodiscard]]
        static wxString GetDashboardScriptInk();
        /// @returns The script handling paging, hashes, and chrome measurement.
        [[nodiscard]]
        static wxString GetDashboardScriptNavigation();
        /// @returns The script counting up large numbers when a page is revealed.
        [[nodiscard]]
        static wxString GetDashboardScriptCounters();
        /// @returns The script revealing pages and highlighting shared colors on hover.
        [[nodiscard]]
        static wxString GetDashboardScriptMotion();
        /// @returns The script for zooming and panning a page with ctrl+wheel/pinch and drag.
        [[nodiscard]]
        static wxString GetDashboardScriptZoom();
        /// @returns The script showing a tooltip for a hovered accessible element.
        [[nodiscard]]
        static wxString GetDashboardScriptTooltips();
        /// @returns The script opening and closing the keyboard and mouse shortcuts panel.
        [[nodiscard]]
        static wxString GetDashboardScriptHelp();
        /// @returns The script saving the current page as a standalone SVG or PNG file.
        [[nodiscard]]
        static wxString GetDashboardScriptSave();
        /// @returns The script wiring up controls and the initial page load.
        [[nodiscard]]
        static wxString GetDashboardScriptEvents();
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_HTML_DASHBOARD_PRINTOUT_H
