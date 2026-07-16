/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PDF_CONTENT_PARSER_H
#define PDF_CONTENT_PARSER_H

#include "pdf_extract_text.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief A PDF affine transformation matrix [a b c d e f], mapping a point
    ///     (x, y) to (a*x + c*y + e, b*x + d*y + f).
    struct pdf_matrix
        {
        /// @brief The matrix's `a` component.
        double m_a{ 1 };
        /// @brief The matrix's `b` component.
        double m_b{ 0 };
        /// @brief The matrix's `c` component.
        double m_c{ 0 };
        /// @brief The matrix's `d` component.
        double m_d{ 1 };
        /// @brief The matrix's `e` component.
        double m_e{ 0 };
        /// @brief The matrix's `f` component.
        double m_f{ 0 };

        /// @param rhs The matrix to concatenate this matrix onto.
        /// @returns This matrix concatenated onto @p rhs (i.e., this * rhs), the
        ///     composition order the `cm` operator and a form's `/Matrix` use.
        [[nodiscard]]
        pdf_matrix multiplied_by(const pdf_matrix& rhs) const noexcept
            {
            return pdf_matrix{ (m_a * rhs.m_a) + (m_b * rhs.m_c),
                               (m_a * rhs.m_b) + (m_b * rhs.m_d),
                               (m_c * rhs.m_a) + (m_d * rhs.m_c),
                               (m_c * rhs.m_b) + (m_d * rhs.m_d),
                               (m_e * rhs.m_a) + (m_f * rhs.m_c) + rhs.m_e,
                               (m_e * rhs.m_b) + (m_f * rhs.m_d) + rhs.m_f };
            }

        /// @brief Transforms a point through the matrix (applies translation).
        /// @param[in,out] xCoord The point's x-coordinate to transform.
        /// @param[in,out] yCoord The point's y-coordinate to transform.
        void transform_point(double& xCoord, double& yCoord) const noexcept
            {
            const double origX{ xCoord };
            xCoord = (m_a * origX) + (m_c * yCoord) + m_e;
            yCoord = (m_b * origX) + (m_d * yCoord) + m_f;
            }

        /// @brief Transforms a delta through the matrix's linear part (no translation).
        /// @param[in,out] xDelta The delta's x-component to transform.
        /// @param[in,out] yDelta The delta's y-component to transform.
        void transform_vector(double& xDelta, double& yDelta) const noexcept
            {
            const double origX{ xDelta };
            xDelta = (m_a * origX) + (m_c * yDelta);
            yDelta = (m_b * origX) + (m_d * yDelta);
            }
        };

    /// @brief The resources (fonts, form XObjects) available to a content stream.
    struct pdf_page_resources
        {
        /// Resource name (e.g., "F1") -> font decoder.
        std::map<std::string, std::shared_ptr<pdf_font_decoder>, std::less<>> m_fonts;
        /// Resource name -> object number of a form XObject.
        std::map<std::string, long, std::less<>> m_xobjects;
        /// Resource name (from /Properties) -> whether that optional-content
        /// group is visible in the document's default configuration.
        std::map<std::string, bool, std::less<>> m_ocg_visible;
        };

    /// @brief Parses page content streams, appending the extracted text to a buffer.
    class pdf_content_parser
        {
      public:
        /// @brief Constructs a parser that appends extracted text to @p text.
        /// @param document The document whose objects and streams are accessed.
        /// @param[in,out] text The output buffer to append decoded text to.
        /// @param hiddenOCGs Object numbers of OCGs that are off in the document's
        ///     default optional-content configuration; marked content belonging to
        ///     one of these is excluded, the same as a viewer would hide it.
        pdf_content_parser(pdf_document& document, std::wstring& text,
                           const std::set<long>& hiddenOCGs);

        /// @brief Extracts the text from one page.
        /// @param pageObject The page object (must have a dictionary).
        void parse_page(const pdf_object& pageObject);

        /// @brief Reverses any still-open RTL run (e.g., one left open at the end
        ///     of a page's content). Safe to call when no run is open.
        void flush_rtl_run();

      private:
        /// @returns @c true if @p character is a bullet-point glyph.
        [[nodiscard]]
        static bool is_bullet(wchar_t character);

        /// @returns @c true if @p character is a strongly right-to-left letter
        ///     (Hebrew or Arabic, including their presentation-form blocks).
        [[nodiscard]]
        static bool is_strong_rtl(wchar_t character);

        /// @returns @c true if @p character is a Hebrew or Arabic combining
        ///     mark (a diacritic that attaches to the character before it).
        [[nodiscard]]
        static bool is_combining_mark(wchar_t character);

        /// @returns The effective line height in text-space units.
        [[nodiscard]]
        double line_height() const;

        /// @brief Appends a line (or paragraph) break to the output.
        void add_newline(bool paragraphBreak);
        /// @brief Appends a space (if the output doesn't already end in whitespace).
        void add_space();
        /// @brief Appends decoded text to the output, normalizing ligatures and
        ///     formatting bullet points that start a line as list items.
        void add_text(const std::wstring& decodedText);
        /// @brief Handles a relative text-position move (Td/TD operators).
        /// @details The operands are offsets in the current text line matrix's
        ///     local space, so they're transformed through it (via
        ///     m_matrixA/B/C/D) into a page-space delta before being handled by
        ///     the same logic as an absolute move (handle_absolute_move).
        void handle_relative_move(double moveX, double moveY);
        /// @brief Handles an absolute text-position move (Tm operator).
        /// @details A move larger than 0.25x the line height along the line-step
        ///     axis (y for horizontal writing mode, x for vertical) starts a new
        ///     line, or a new paragraph if larger than 1.8x.
        ///     If the move lands on the same line but far enough away, a space is
        ///     inserted so the two runs don't run together. This can happen with a
        ///     separate BT/Tm/Tj block placed well to the right, or diagonally for
        ///     rotated text.
        /// @returns @c true if a newline was written (i.e., @p newX/@p newY landed
        ///     on a different line than the current position).
        bool handle_absolute_move(double newX, double newY);
        /// @brief Decodes and appends a shown string (the Tj, ', " operators).
        void show_string(const std::string& stringBytes, const pdf_font_decoder* currentFont);
        /// @brief Decodes and appends a TJ array (strings mixed with kerning values).
        /// @details A kerning value's displayed width is scaled by the current
        ///     horizontal scaling (Tz) before being checked against the word-gap threshold.
        void show_array(std::string_view arrayValue, const pdf_font_decoder* currentFont);

        /// @brief Loads the resources (fonts and form XObjects) available
        ///     to a content stream.
        [[nodiscard]]
        pdf_page_resources load_resources(std::string_view resourcesValue);

        /// @brief Finds a key in a page's dictionary, walking up the page tree
        ///     if the page inherits it.
        [[nodiscard]]
        std::string_view find_inherited_value(const pdf_object& pageObject,
                                              std::string_view keyName) const;

        /// @returns A page's (decoded) content, concatenated from its
        ///     `/Contents` stream(s).
        [[nodiscard]]
        std::string load_page_content(const pdf_object& pageObject) const;

        /// @brief Tokenizes a content stream, tracking the text operators.
        /// @details PDF content streams are stacks of operands followed by an operator
        ///     (e.g., `12 Tf` sets the font). This function accumulates numeric and
        ///     string operands until an operator consumes them.
        ///     @p depth limits recursion into form XObjects to prevent stack overflows
        ///     on malformed files.
        void parse_content(std::string_view content, const pdf_page_resources& resources,
                           int depth);

        /// @returns The linear (a, b, c, d) part of the current text line matrix
        ///     (m_matrixA/B/C/D) concatenated with the CTM (m_ctm), as a
        ///     translation-free matrix. Text-space deltas and scales are
        ///     transformed through this to reach page space.
        [[nodiscard]]
        pdf_matrix combined_linear() const noexcept
            {
            const pdf_matrix textLineMatrix{ m_matrixA, m_matrixB, m_matrixC, m_matrixD, 0, 0 };
            return textLineMatrix.multiplied_by(
                pdf_matrix{ m_ctm.m_a, m_ctm.m_b, m_ctm.m_c, m_ctm.m_d, 0, 0 });
            }

        /// @brief Reads a 6-number matrix array value (e.g., a form XObject's
        ///     `/Matrix`).
        /// @returns The matrix, or identity if @p matrixValue is absent or malformed.
        [[nodiscard]]
        static pdf_matrix read_matrix_value(std::string_view matrixValue);

        /// @brief A snapshot of the graphics state that `q` saves and `Q` restores.
        struct graphics_state_snapshot
            {
            pdf_matrix m_ctm;
            double m_fontSize{ 12 };
            double m_fontScale{ 1 };
            double m_leading{ 0 };
            double m_horizScale{ 100 };
            bool m_verticalWritingMode{ false };
            const pdf_font_decoder* m_font{ nullptr };
            };

        pdf_document& m_document;            ///< Document being parsed.
        std::wstring& m_text;                ///< Output buffer (owned by the caller).
        const std::set<long>& m_hidden_ocgs; ///< Object numbers of OCGs that are off by default.
        std::set<long> m_visited_xobjects;   ///< XObjects already recursed into (cycle guard).
        /// Marked-content nesting stack (pushed by BDC/BMC, popped by EMC); each
        /// entry is whether that level (or an ancestor) is an OCG that's hidden.
        std::vector<bool> m_markedContentHidden;
        double m_currentX{ 0 };  ///< Current horizontal position in user-space.
        double m_currentY{ 0 };  ///< Current vertical position in user-space.
        double m_fontSize{ 12 }; ///< Current font size (from Tf operator).
        double m_fontScale{ 1 }; ///< Vertical scale factor from the text matrix.
        /// The a, b, c, d components of the current text line matrix (from Tm; reset
        /// to identity by BT). Used to transform a Td/TD's local-space operands into
        /// a page-space delta when the matrix carries rotation or scale.
        double m_matrixA{ 1 };
        double m_matrixB{ 0 };
        double m_matrixC{ 0 };
        double m_matrixD{ 1 };
        /// Current transformation matrix, built up from `cm` operators and form
        /// XObject `/Matrix` entries and saved/restored by `q`/`Q`. Text-space
        /// coordinates are mapped through this into page space before any
        /// line-break decision. Survives BT/ET (unlike the text line matrix).
        pdf_matrix m_ctm;
        /// Whether the current font (from Tf) lays out text in vertical writing mode
        /// (its /Encoding is one of Adobe's predefined "-V" CMaps), so a line step is
        /// a horizontal move across columns rather than a vertical move down the page.
        bool m_verticalWritingMode{ false };
        double m_leading{ 0 };         ///< Current leading (line spacing, from TL).
        double m_horizScale{ 100 };    ///< Current horizontal scaling percent (from Tz).
        bool m_haveY{ false };         ///< Whether m_currentY has been initialized.
        bool m_atLineStart{ true };    ///< True when no glyphs emitted since the last newline.
        bool m_haveShownText{ false }; ///< Whether any glyph has been shown on this page yet.
        /// True between a `BT` operator and the first `Td`/`TD`/`Tm` after it (i.e.,
        /// while the text line matrix is still at its just-reset identity value).
        bool m_freshTextObject{ true };
        /// Start of the currently open RTL run in m_text (std::wstring::npos if
        /// no run is open). Spans from the first strongly-RTL character seen to
        /// the most recent one, so that interior whitespace (e.g., between RTL
        /// words) is reversed along with them, while trailing whitespace after
        /// the run's last RTL character is left in place.
        size_t m_rtlRunStart{ std::wstring::npos };
        /// One-past the most recent strongly-RTL character in the open run.
        size_t m_rtlRunEnd{ std::wstring::npos };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // PDF_CONTENT_PARSER_H
