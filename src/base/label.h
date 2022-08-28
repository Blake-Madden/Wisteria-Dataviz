/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_CANVASLABEL_H__
#define __WISTERIA_CANVASLABEL_H__

#include <vector>
#include <string_view>
#include <wx/wx.h>
#include <wx/tokenzr.h>
#include <wx/fontenum.h>
#include <wx/regex.h>
#include "graphitems.h"

// forward declares
namespace Wisteria
    {
    class Canvas;
    }

namespace Wisteria::Graphs
    {
    class Graph2D;
    class HeatMap;
    }

namespace Wisteria::GraphItems
    {
    class Points2D;
    class GraphItemBase;
    class Axis;
    }

namespace Wisteria::GraphItems
    {
    /** @brief A text box that can be placed on a canvas. Can also be used as a legend.
        @note Call GetLabelDisplayInfo() to edit most of the appearance and layout
            functionality for a label.
        @sa The [label](../../Labels.md) overview for more information.*/
    class Label final : public GraphItemBase
        {
        friend class Graphs::Graph2D;
        friend class Graphs::HeatMap;
        friend class Wisteria::Canvas;
        friend class Axis;
        friend class Points2D;
        friend class GraphItemBase;
    public:
        /// @private
        Label() noexcept
            {
            ShowLabelWhenSelected(false);
            GetPen() = wxNullPen;
            }
        /** @brief Constructor.
            @param itemInfo Base plot object settings.*/
        explicit Label(const GraphItems::GraphItemInfo& itemInfo) :
              GraphItemBase(itemInfo)
            {
            ShowLabelWhenSelected(false);
            InvalidateCachedBoundingBox();
            CalcLongestLineLength();
            }
        /** @brief Constructor.
            @param text The text to display on the label.*/
        explicit Label(const wxString& text) :
              GraphItemBase(1.0f, text)
            {
            GetPen() = wxNullPen;
            ShowLabelWhenSelected(false);
            InvalidateCachedBoundingBox();
            CalcLongestLineLength();
            }

        /** @brief Chop the label's text up so that it will fit within a bounding box.
            @param dc The device context to measure with.
            @param boundingBoxSize The size of the bounding box to fit the text into.
            @note If the bounding box isn't tall enough to fit the text, then the text
                will be truncated and have an ellipsis appended to it.*/
        void SplitTextToFitBoundingBox(wxDC& dc, const wxSize& boundingBoxSize);

        /** @brief Splits the label into multiline chunks, with each line being around
             the suggested length argument.
            @details String will be split on these delimiters: spaces and hyphens.
            @param suggestedLineLength The suggested length (character count) for each new line.*/
        void SplitTextToFitLength(const size_t suggestedLineLength);

        /** @brief Attempts to split the label into two lines if a logical break in the text
                can be found.
            @details String will be split on these delimiters:
                open parenthesis, open brace, forward slash, ampersand, or colon. If only two
                words, then will split on the space between them.
            @returns @c true if the text was successfully split.*/
        bool SplitTextAuto();

        /** @brief Attempts to split the label into lines if a list of items
                (commas followed by spaces).
            @details For example, "Arts, Humanities, & Communications" would be split into
                "Arts,\nHumanities, &\nCommunications".
            @note The "and/or" after the last comma will be included at the end of the
                second-to-last line. Along with ampersand, "and" and "or" for English,
                Spanish, French, and German are supported.
            @returns @c true if the text was successfully split.*/
        bool SplitTextByListItems();

        /** @brief Splits the text into lines containing only one character.
            @note It is recommended to set the alignment to centered for best appearance.
            @par Example
            @code
             // set the left title of a plot to be written downward,
             // letter-by-letter
             thePlot->GetLeftYAxis().GetTitle().GetGraphItemInfo().
                Label(L"TIME").Orient(Orientation::Horizontal).
                LabelAlignment(TextAlignment::Centered);
             thePlot->GetLeftYAxis().GetTitle().SplitTextByCharacter();
            @endcode*/
        void SplitTextByCharacter();

        /// @returns The minimum width needed for the left padding if including a legend.
        /// @sa SetLeftPadding().
        [[nodiscard]] static wxCoord GetMinLegendWidthDIPs() noexcept
            {
            return Wisteria::Icons::LegendIcon::GetIconWidthDIPs() +
                   2/* 1 DIP on each side of icon*/;
            }
        /** @returns The number of pixels between lines.
            @warning This will need to be scaled when being drawn or measured.*/
        [[nodiscard]] double GetLineSpacing() const noexcept
            { return m_spacingBetweenLines; }
        /** @brief Sets the number of DIPs between lines (if label is multiline).
            @param spacing The number of DIPs to space between lines.
            @note This is in DIPS; the framework will scale this to the current DPI and zoom
                level for you.*/
        void SetLineSpacing(const double spacing) noexcept
            { m_spacingBetweenLines = spacing; }

        /// @returns How the corners are drawn.
        [[nodiscard]] BoxCorners GetBoxCorners() const noexcept
            { return m_boxCorners; }
        /** @brief Sets how the corners are drawn.
            @details Only relevant if drawing an outline.
            @param boxCorners The corner display to use.*/
        void SetBoxCorners(const BoxCorners boxCorners) noexcept
            { m_boxCorners = boxCorners; }

        /** @brief Tilts the text by the provided degree.
            @param tiltAngle The angle to tilt the text.
            @warning The bounding box of the label will not take this tilt into account.
                This will enable vertical labels with a slight tilt to blend with each other
                without creating large negative spaces between them.*/
        void Tilt(const double tiltAngle) noexcept
            { m_tiltAngle = tiltAngle; }

        /** @brief Sets the label, which the caller can use (e.g., as a selection label).
            @param label The text for the label.*/
        void SetText(const wxString& label) final
            {
            GraphItemBase::SetText(label);
            CalcLongestLineLength();
            InvalidateCachedBoundingBox();
            }
        /** @brief Changes the text at the given line in the label.
            @param line The line number to change.
            @param lineText The text to replace the specified line number with.
            @note If the provided line index is out of range, then nothing is updated.*/
        void SetLine(const size_t line, const wxString& lineText);

        /** @brief Draws the box onto the given @c wxDC.
            @param dc The wxDC to render onto.
            @returns The box that the text is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;

        /** @returns The rectangle on the canvas where the label would fit in.
            @param dc An existing graphics context to measure the label on.
            @note This is a more optimal alternative to GetBoundingBox(), which doesn't have to
                create its own temporary @c wxDC.*/
        [[nodiscard]] wxRect GetBoundingBox(wxDC& dc) const;
        /** @brief Bounds the label to be within the given rectangle.
            @param rect The rectangle to bound the label to.
            @param dc The DC to measure content with.
            @param parentScaling The parent's scaling (not used in this implementation).
            @note The scaling of the label will be adjusted to this box,
             and will also be anchored (length-wise if vertical, height-wise if horizontal)
             within this box.\n
             If the label isn't wide/tall enough to fill the bounding box precisely
             width- and length-wise, then it will be anchored.\n
             Call SetAnchoring() to control how it is anchored.*/
        void SetBoundingBox(const wxRect& rect, wxDC& dc, [[maybe_unused]] const double parentScaling) final;

        /** @brief Moves the item by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final
            {
            SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove));
            InvalidateCachedBoundingBox();
            }

        /// @brief Gets/sets the lines that are drawn ignoring the left margin.
        ///  This is useful for legend formatting.
        /// @returns The lines ignoring (left) margins.
        [[nodiscard]] std::set<size_t>& GetLinesIgnoringLeftMargin() noexcept
            { return m_linesIgnoringLeftMargin; }
        /// @private
        [[nodiscard]] const std::set<size_t>& GetLinesIgnoringLeftMargin() const noexcept
            { return m_linesIgnoringLeftMargin; }

        /** @brief Set this to @c true so that calls to SetBoundingBox() will only
                be treated as a suggestion. The bounding box will be set to the suggested size,
                but then be scaled down to the content.
            @details This behaviour is turned off by default, so that calls to SetBoundingBox()
                will explicitly set the size.\n
                This is mostly useful for legends being embedded on a canvas. Having this option
                set will tell the canvas to re-measure the legend and only use the space that it
                needs, giving any extra space back to the items in the same row. If this is not
                set, then the canvas will just use the pre-calculated size of the legend based
                on its proportion to the overall canvas; thus, it may upscale the legend's size,
                instead of keeping it at the correct scale and reclaiming the extra space.\n
                This situation relates to when the canvas is being made wider than when
                its canvas proportion was calculated.
             @param adjust @c true to tell SetBoundingBox() to only treat its size
                as a suggestion.*/
        void AdjustingBoundingBoxToContent(const bool adjust) noexcept
            { m_adjustBoundingBoxToContent = adjust; }
        /// @returns @c true if the bounding box passed to SetBoundingBox() is only
        ///  treated as a suggestion.
        [[nodiscard]] bool IsAdjustingBoundingBoxToContent() const noexcept
            { return m_adjustBoundingBoxToContent; }

        /// @name Font Functions
        /// @brief Helper functions for font selection and adjustments.
        /// @note To change or edit the font for a label, call
        ///     @c GetGraphItemInfo().Font() or GetFont().
        /// @{

        /** @returns The best font size to fit a given string across an area diagonally.
            @param dc The device context to use for measuring.
            @param ft The font that will be used to measure with.
            @param boundingBox The bounding box to constrain the text within.
            @param angleInDegrees The angle that the string will be at.
            @param text The string being measured.
            @warning @c boundingBoxSize is assumed to be scaled from the parent already,
             so the font size returned will fit this area as-is.\n
             Because `Label`s adjust their font size based on scaling,
             setting its point size to this should also set its scaling to 1 (not the parents').*/
        [[nodiscard]] static int CalcDiagonalFontSize(wxDC& dc,
                                                      const wxFont& ft, const wxRect& boundingBox,
                                                      const double angleInDegrees,
                                                      const wxString& text);
        /** @returns The font size that would fit for a given string within a given bounding box.
            @param dc The device context that the text is being drawn on.
            @param ft The font this is being drawn with.
            @param boundingBox The bounding box that the text should fit inside of.
            @param text The text that is being drawn.
            @warning `boundingBoxSize` is assumed to be scaled from the parent already,
             so the font size returned will fit this area as-is.\n
             Because `Label`s adjust their font size based on scaling,
             setting its point size to this should also set its scaling to 1 (not the parents')*/
        [[nodiscard]] static int CalcFontSizeToFitBoundingBox(wxDC& dc, const wxFont& ft,
            const wxRect& boundingBox, const wxString& text);
        /** @returns The first available font (face name) found from the given list,
             or the system default if none are found.
            @param possibleFontNames The list of font names to choose from.*/
        [[nodiscard]] static wxString GetFirstAvailableFont(const wxArrayString& possibleFontNames);
        /** @returns The first available cursive font (face name) found on the system,
             or the system default if none are found.
            @note This function uses a list of known cursive fonts to search with.*/
        [[nodiscard]] static wxString GetFirstAvailableCursiveFont();
        /// @brief Corrects issues with fonts such as bogus facenames and point sizes.
        /// @param theFont The font to review and correct.
        static void FixFont(wxFont& theFont);
        /// @}
    private:
        /// @returns Number of lines of text in the label.
        [[nodiscard]] size_t GetLineCount() const noexcept
            { return m_lineCount; }
        /// @returns Number of lines of text in the label, ignoring the header (if enabled).
        [[nodiscard]] size_t GetLineCountWithoutHeader() const noexcept
            { return m_lineCount - (GetHeaderInfo().IsEnabled() ? 1 : 0); }
        /// @returns The number of characters from the longest line of text in the label.
        [[nodiscard]] size_t GetLongestLineLength() const noexcept
            {
            // make sure this was cached properly
            wxASSERT_LEVEL_2_MSG(
                (GetText().length() == 0 && m_longestLineLength == 0) ||
                (GetText().length() > 0 && m_longestLineLength > 0),
                L"Longest line length in label was not calculated!");
            return m_longestLineLength;
            }
        /** @returns @c true if the given point is inside of the label.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final
            { return GetBoundingBox(dc).Contains(pt); }
        /** @brief Draws a vertical multi-line text string at the specified point,
             using the current text font, and the current text foreground and background colours.
            @param dc The device context to draw to.
            @param pt The point to draw the text.\n
             This coordinate refers to the bottom-left corner of the rectangle bounding the string.*/
        void DrawVerticalMultiLineText(wxDC& dc, wxPoint pt) const;
        /** @brief Draws a multi-line text string at the specified point, using the current text font,
             and the current text foreground and background colours.
            @param dc The device context that the text is being drawn on.
            @param pt The point to draw the text.\n
             This coordinate refers to the top-left corner of the rectangle bounding the string.*/
        void DrawMultiLineText(wxDC& dc, wxPoint pt) const;
        /// @brief Figures out how many characters are in the longest line of text
        ///  (takes multiline labels into account).
        void CalcLongestLineLength();
        /** @brief Retrieves The physical size of label
             (including outlined bounding box if the pen is valid).
            @details Should only be called by GetBoundingBox(), which avoids calling this if
             the bounding box values are already cached. GetBoundingBox() will also take into
             account the (optional) minimum size of the label in conjunction with this.*/
        void GetSize(wxDC& dc, wxCoord& width, wxCoord& height) const;
        /// @returns The offset from the top if user-defined minimum size is being used it is
        ///  taller than the measured size.
        [[nodiscard]] wxCoord CalcPageVerticalOffset(wxDC& dc) const;
        /// @returns The offset from the left if user-defined minimum size is being used it is
        ///  wider than the measured size.
        [[nodiscard]] wxCoord CalcPageHorizontalOffset(wxDC& dc) const;

        double m_tiltAngle{ 0 };
        double m_spacingBetweenLines{ 1 };
        bool m_adjustBoundingBoxToContent{ false };
        size_t m_lineCount{ 0 };
        size_t m_longestLineLength{ 0 };
        std::set<size_t> m_linesIgnoringLeftMargin;
        BoxCorners m_boxCorners{ BoxCorners::Straight };
        };
    }

/** @}*/

#endif //__WISTERIA_CANVASLABEL_H__
