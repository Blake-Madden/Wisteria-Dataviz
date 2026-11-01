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
#include "reportexportpage.h"
#include <optional>
#include <string_view>
#include <vector>
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
        /// @brief A row of a table on a slide.
        struct TableSlideRow
            {
            /// @brief The row's cells.
            std::vector<wxString> m_cells;
            /// @brief The horizontal alignment of the row's cells.
            std::vector<wxAlignment> m_alignments;
            /// @brief The row's height, in inches.
            double m_height{ 0.0 };
            /// @brief Whether the row has the band color. All rows that a long row
            ///     was split into have the same color.
            bool m_isBanded{ false };
            };

        /// @brief The part of a table shown on one slide.
        struct TableSlide
            {
            /// @brief The title above the table.
            wxString m_title;
            /// @brief The columns.
            std::vector<ReportTableColumn> m_columns;
            /// @brief The width of each column, in inches.
            std::vector<double> m_columnWidths;
            /// @brief The rows on this slide.
            std::vector<TableSlideRow> m_rows;
            /// @brief The space on the left and right of a cell's text, in inches.
            double m_cellMarginX{ 0.0 };
            /// @brief The font size of the cells, in points.
            int m_fontPoints{ 0 };
            /// @brief The height of the header row, in inches.
            double m_headerHeight{ 0.0 };
            /// @brief The distance from the top of the slide to the table, in inches.
            double m_top{ 0.0 };
            /// @brief The slide width, in inches.
            double m_slideWidth{ 0.0 };
            /// @brief The slide height, in inches.
            double m_slideHeight{ 0.0 };
            /// @brief The watermark, with its lengths and font size in points.
            std::optional<ReportWatermarkLayout> m_watermark;

            /// @returns The height of the table on this slide, in inches.
            [[nodiscard]]
            double GetTableHeight() const
                {
                double tableHeight{ m_headerHeight };
                for (const auto& row : m_rows)
                    {
                    tableHeight += row.m_height;
                    }
                return tableHeight;
                }
            };

        /// @brief The slide margin around a table, in inches.
        constexpr static double TABLE_MARGIN_INCHES{ 0.4 };
        /// @brief The height of the title above a table, in inches.
        constexpr static double TABLE_TITLE_HEIGHT_INCHES{ 0.6 };
        /// @brief The font size of the title above a table, in points.
        constexpr static int TABLE_TITLE_FONT_POINTS{ 24 };
        /// @brief The most space on the left and right of a table cell's text, in inches.
        constexpr static double TABLE_CELL_MARGIN_X_INCHES{ 0.1 };
        /// @brief The space above and below a table cell's text, in inches.
        constexpr static double TABLE_CELL_MARGIN_Y_INCHES{ 0.05 };
        /// @brief The table header's background color, as @c "RRGGBB".
        constexpr static std::wstring_view TABLE_HEADER_COLOR{ L"E0E0E0" };
        /// @brief The background color of every other table row, as @c "RRGGBB".
        constexpr static std::wstring_view TABLE_BAND_COLOR{ L"F2F2F2" };
        /// @brief The color of the lines between table rows, as @c "RRGGBB".
        constexpr static std::wstring_view TABLE_LINE_COLOR{ L"BFBFBF" };

        /// @brief One rendered page, ready to be written into a package.
        struct RenderedPage
            {
            /// @brief The page's name. If empty, then its page number is used.
            wxString m_label;
            /// @brief The native table that this page shows, instead of a picture.
            std::optional<TableSlide> m_table;
            /// @brief The page rendered as SVG.
            wxString m_svg;
            /// @brief The page rendered as PNG.
            wxMemoryBuffer m_png;
            /// @brief The width of the rendered page, in pixels.
            int m_pixelWidth{ 0 };
            /// @brief The height of the rendered page, in pixels.
            int m_pixelHeight{ 0 };
            /// @brief The speaker notes for the page.
            wxString m_notes;
            };

        /// @brief Escapes text for XML element content or attribute values and drops the
        ///     control characters that are illegal in XML 1.0.
        /// @param str The text to escape.
        /// @returns The escaped text.
        [[nodiscard]]
        static wxString EscapeXml(const wxString& str);
        /// @brief Escapes text for a single-line XML attribute value. Collapses embedded
        ///     newlines/tabs down to single spaces before escaping.
        /// @param str The text to escape.
        /// @returns The escaped text.
        [[nodiscard]]
        static wxString EscapeXmlAttribute(const wxString& str);
        /// @brief Splits a list or grid control into slides.
        /// @param page The page holding the control.
        /// @param slideWidthInches The slide width, in inches.
        /// @param slideHeightInches The slide height, in inches.
        /// @returns The table's slides. This is empty if the page has no table data.
        [[nodiscard]]
        static std::vector<TableSlide> BuildTableSlides(const ReportExportPage& page,
                                                        double slideWidthInches,
                                                        double slideHeightInches);
        /// @brief Gathers a slide's speaker-notes text from the canvas titles and from every
        ///     fixed object's accessibility label (user override first, then the
        ///     auto-generated description). Paragraphs are separated by blank lines.
        /// @param canvas The canvas to gather text from.
        /// @returns The combined speaker-notes text.
        [[nodiscard]]
        static wxString CollectAccessibilityText(Canvas* canvas);
        /// @brief Renders one canvas to an in-memory SVG document and, optionally, in-memory
        ///     PNG bytes, laid out at renderSize (DIPs). The canvas is temporarily resized for
        ///     the render and restored afterward.
        /// @param canvas The canvas to render.
        /// @param renderSize The size to lay out the canvas at, in DIPs.
        /// @param[out] svgOut Receives the rendered SVG document.
        /// @param[out] pngOut Receives the rendered PNG bytes, or @c nullptr to skip the
        ///     raster render entirely.
        static void RenderCanvas(Canvas* canvas, wxSize renderSize, wxString& svgOut,
                                 wxMemoryBuffer* pngOut = nullptr);
        /// @param color The color to convert.
        /// @returns @p color as an uppercase @c "RRGGBB" hex string (no leading @c '#').
        [[nodiscard]]
        static wxString ColorToHex(const wxColour& color);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_SLIDE_EXPORT_BASE_H
