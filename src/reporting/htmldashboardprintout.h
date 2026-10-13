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
#include <memory>
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
            Slides,
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
        /// @brief Whether each page is also rendered with its width and height swapped.
        /// @details The browser shows whichever orientation best fits its window.
        bool m_dualOrientations{ true };
        /// @brief Whether to include the Save button.
        bool m_includeSave{ true };
        /// @brief Whether to include the Print button.
        bool m_includePrint{ true };
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
            return (view == DashboardView::Gallery) ? L"gallery" : L"slides";
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
            if (str == L"slides")
                {
                return DashboardView::Slides;
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

        /// @brief Enables/disables rendering each page in both orientations.
        /// @param dual @c true to also render each page with its width and height swapped.
        /// @returns A reference to this object.
        HtmlDashboardOptions& DualOrientations(const bool dual) noexcept
            {
            m_dualOrientations = dual;
            return *this;
            }

        /// @brief Enables/disables the Save button.
        /// @param include @c true to include the button.
        /// @returns A reference to this object.
        HtmlDashboardOptions& IncludeSave(const bool include) noexcept
            {
            m_includeSave = include;
            return *this;
            }

        /// @brief Enables/disables the Print button.
        /// @param include @c true to include the button.
        /// @returns A reference to this object.
        HtmlDashboardOptions& IncludePrint(const bool include) noexcept
            {
            m_includePrint = include;
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
        /// @returns The script switching between the versions of a page with filterable graphs.
        [[nodiscard]]
        static wxString GetDashboardScriptChoices();
        /// @returns The script showing a page's selected filter choice.
        [[nodiscard]]
        static wxString GetDashboardScriptChoiceSwitch();
        /// @returns The script handling page visibility, layers, and the URL hash.
        [[nodiscard]]
        static wxString GetDashboardScriptPages();
        /// @returns The script building the gallery cards and rendering the current view.
        [[nodiscard]]
        static wxString GetDashboardScriptGallery();
        /// @returns The script showing/hiding tagged graph items.
        [[nodiscard]]
        static wxString GetDashboardScriptFilters();
        /// @returns The script gathering a page's filterable items into sections.
        [[nodiscard]]
        static wxString GetDashboardScriptFilterSections();
        /// @returns The script building the Filters menu.
        [[nodiscard]]
        static wxString GetDashboardScriptFilterMenu();
        /// @returns The script tagging shapes so their painted color survives dark mode.
        [[nodiscard]]
        static wxString GetDashboardScriptInk();
        /// @returns The script handling paging, hashes, and chrome measurement.
        [[nodiscard]]
        static wxString GetDashboardScriptNavigation();
        /// @returns The script counting up large numbers when a page is revealed.
        [[nodiscard]]
        static wxString GetDashboardScriptCounters();
        /// @returns The script classifying and keying a page's marks.
        [[nodiscard]]
        static wxString GetDashboardScriptMarks();
        /// @returns The script preparing a page's marks for animation.
        [[nodiscard]]
        static wxString GetDashboardScriptMotion();
        /// @returns The script highlighting shared colors on hover.
        [[nodiscard]]
        static wxString GetDashboardScriptSpotlight();
        /// @returns The script revealing pages as they scroll into view.
        [[nodiscard]]
        static wxString GetDashboardScriptReveal();
        /// @returns The script for zooming and panning a page with ctrl+wheel/pinch and drag.
        [[nodiscard]]
        static wxString GetDashboardScriptZoom();
        /// @returns The script showing a tooltip for a hovered accessible element.
        [[nodiscard]]
        static wxString GetDashboardScriptTooltips();
        /// @returns The script opening and closing the keyboard and mouse shortcuts panel.
        [[nodiscard]]
        static wxString GetDashboardScriptHelp();
        /// @returns The script making a standalone copy of a page's SVG.
        [[nodiscard]]
        static wxString GetDashboardScriptSaveCopy();
        /// @returns The script saving the current page as a standalone SVG or PNG file.
        [[nodiscard]]
        static wxString GetDashboardScriptSave();
        /// @returns The script for the Save button's menu.
        [[nodiscard]]
        static wxString GetDashboardScriptSaveMenu();
        /// @returns The script printing the current page in its viewed orientation.
        [[nodiscard]]
        static wxString GetDashboardScriptPrint();
        /// @returns The script for toggling fullscreen presentation mode.
        [[nodiscard]]
        static wxString GetDashboardScriptPresent();
        /// @returns The script for opening, closing, and keyboard navigation of the theme menu.
        [[nodiscard]]
        static wxString GetDashboardScriptThemeMenu();
        /// @returns The script wiring up the theme button and menu items.
        [[nodiscard]]
        static wxString GetDashboardScriptTheme();
        /// @returns The script handling keyboard navigation and drop-down menus.
        [[nodiscard]]
        static wxString GetDashboardScriptKeys();
        /// @returns The script wiring up controls.
        [[nodiscard]]
        static wxString GetDashboardScriptEvents();
        /// @returns The script performing the initial page load.
        [[nodiscard]]
        static wxString GetDashboardScriptLoad();

      private:
        using FilterableGraphs = std::vector<std::shared_ptr<Graphs::FilterableGraph>>;

        /// @brief How a filterable graph was shown on the canvas before exporting.
        struct FilterState
            {
            size_t m_activeIndex{ 0 };
            bool m_shown{ true };
            bool m_legendShown{ true };
            };

        [[nodiscard]]
        wxString BuildLogo(HtmlDashboardOptions options);
        [[nodiscard]]
        wxString BuildKeyboardTip();

        /// @returns The filterable graphs on a page that are switched together.
        /// @param canvas The page.
        [[nodiscard]]
        static FilterableGraphs GetPageFilters(const Canvas& canvas);
        /// @returns @c true if any of the filters includes an unfiltered choice.
        /// @param pageFilters The page's filterable graphs.
        [[nodiscard]]
        static bool HasAllChoice(const FilterableGraphs& pageFilters);
        /// @returns The page's filter choices, with the unfiltered choice first (if included).
        /// @param pageFilters The page's filterable graphs.
        [[nodiscard]]
        static std::vector<wxString> GetFilterChoices(const FilterableGraphs& pageFilters);
        /// @returns The index of the choice being shown on the canvas.
        /// @param pageFilters The page's filterable graphs.
        /// @param choices The page's filter choices.
        [[nodiscard]]
        static size_t GetDefaultFilterChoice(const FilterableGraphs& pageFilters,
                                             const std::vector<wxString>& choices);
        /** @brief Shows a filter choice on all of a page's filterable graphs.
            @details Graphs that don't have the choice (and their legends) are left blank.
            @param pageFilters The page's filterable graphs.
            @param originalStates How the graphs were originally shown.
            @param choice The filter value to show.
            @param isAllChoice @c true if showing the unfiltered choice.*/
        static void ShowFilterChoice(const FilterableGraphs& pageFilters,
                                     const std::vector<FilterState>& originalStates,
                                     const wxString& choice, bool isAllChoice);
        /** @brief Makes the element IDs in a rendered page unique within the dashboard.
            @details The SVG DC numbers its IDs (gradients, clip paths, etc.) from zero for
                every render and reuses hatch pattern names, so each ID is prefixed with
                @c scope (and numbered if repeated). References point to the most recent
                definition before them.
            @param svg The rendered SVG content.
            @param scope A prefix unique to this render.
            @returns The SVG content with scoped IDs.*/
        [[nodiscard]]
        static wxString ScopeSvgIds(const wxString& svg, const wxString& scope);
        /// @brief Restores how a page's filterable graphs were originally shown.
        /// @param pageFilters The page's filterable graphs.
        /// @param originalStates How the graphs were originally shown.
        static void RestoreFilterStates(const FilterableGraphs& pageFilters,
                                        const std::vector<FilterState>& originalStates);
        /// @returns The caption describing the filters shown on a page,
        ///     or an empty string if it has none.
        /// @param canvas The page.
        [[nodiscard]]
        static wxString GetFilterCaption(const Canvas& canvas);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_HTML_DASHBOARD_PRINTOUT_H
