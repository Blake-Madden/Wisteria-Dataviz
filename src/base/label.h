/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
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
            GetGraphItemInfo().Outline(true, true, true, true);
            ShowLabelWhenSelected(false);
            GetPen() = wxNullPen;
            }
        /** @brief Constructor.
            @param itemInfo Base plot object settings.*/
        explicit Label(const GraphItems::GraphItemInfo& itemInfo) :
              GraphItemBase(itemInfo)
            {
            GetGraphItemInfo().Outline(true, true, true, true);
            ShowLabelWhenSelected(false);
            InvalidateCachedBoundingBox();
            CalcLongestLineLength();
            }
        /** @brief Constructor.
            @param text The text to display on the label.*/
        explicit Label(const wxString& text) :
              GraphItemBase(1.0f, text)
            {
            GetGraphItemInfo().Outline(true, true, true, true);
            GetPen() = wxNullPen;
            ShowLabelWhenSelected(false);
            InvalidateCachedBoundingBox();
            CalcLongestLineLength();
            }

        /** @brief Draws the box onto the given @c wxDC.
            @param dc The wxDC to render onto.
            @returns The box that the text is being drawn in.*/
        wxRect Draw(wxDC& dc) const final;

        /// @returns The minimum width needed for the left padding if including a legend.
        /// @sa SetLeftPadding().
        [[nodiscard]]
        static constexpr wxCoord GetMinLegendWidthDIPs() noexcept
            {
            return Wisteria::Icons::LegendIcon::GetIconWidthDIPs() +
                   2/* 1 DIP on each side of icon*/;
            }

        /// @brief Gets/sets the lines that are drawn ignoring the left margin.
        ///     This is useful for legend formatting.
        /// @returns The lines ignoring (left) margins.
        [[nodiscard]]
        std::set<size_t>& GetLinesIgnoringLeftMargin() noexcept
            { return m_linesIgnoringLeftMargin; }
        /// @private
        [[nodiscard]]
        const std::set<size_t>& GetLinesIgnoringLeftMargin() const noexcept
            { return m_linesIgnoringLeftMargin; }

        /// @name Text Functions
        /// @brief Functions for settings and editing the label's text.
        /// @{

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

        /** @brief Attempts to split the label into lines, using conjunctions as the separator.
            @details For example, "Arts and Humanities and Communications" would be split into
                "Arts and \nHumanities and\nCommunications".
            @note Along with ampersand, "and" and "or" for English,
                Spanish, French, and German are supported.
            @returns @c true if the text was successfully split.
            @sa SplitTextByListItems() for splitting on commas also.*/
        bool SplitTextByConjunctions();

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

        /// @returns Number of lines of text in the label.
        [[nodiscard]]
        size_t GetLineCount() const noexcept
            { return m_lineCount; }
        /// @returns Number of lines of text in the label, ignoring the header (if enabled).
        [[nodiscard]]
        size_t GetLineCountWithoutHeader() const noexcept
            { return m_lineCount - (GetHeaderInfo().IsEnabled() ? 1 : 0); }
        /// @returns The number of characters from the longest line of text in the label.
        [[nodiscard]]
        size_t GetLongestLineLength() const noexcept
            {
            // make sure this was cached properly
            wxASSERT_LEVEL_2_MSG(
                (GetText().length() == 0 && m_longestLineLength == 0) ||
                (GetText().length() > 0 && m_longestLineLength > 0),
                L"Longest line length in label was not calculated!");
            return m_longestLineLength;
            }
        /// @}

        /// @name Style Functions
        /// @brief Functions for customizing the label's visual appearance.
        /// @{

        /// @returns How the corners are drawn.
        [[nodiscard]]
        BoxCorners GetBoxCorners() const noexcept
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

        /** @returns The number of pixels between lines.
            @warning This will need to be scaled when being drawn or measured.*/
        [[nodiscard]]
        double GetLineSpacing() const noexcept
            { return m_spacingBetweenLines; }
        /** @brief Sets the number of DIPs between lines (if label is multiline).
            @param spacing The number of DIPs to space between lines.
            @note This is in DIPS; the framework will scale this to the current DPI and zoom
                level for you.*/
        void SetLineSpacing(const double spacing) noexcept
            { m_spacingBetweenLines = spacing; }

        /** @brief Adds an image to the left side of the text.
            @details This is a shortcut for creating a whole-image legend for the label.\n
                Because of this, the image's size will be the default icon width and
                the height will be scaled to the height of the text (preserving the aspect ratio).
            @warning This will reset any legend that the label currently has.
            @param bmp The image to use.*/
        void SetLeftImage(const wxBitmapBundle& bmp)
            { m_leftImage = bmp; }
        /** @brief Adds an image to the top side of the text.
            @details The image's size will be its original size,
                but the width will be scaled to the width of the text if necessary
                (preserving the aspect ratio).
            @warning This will reset any legend that the label currently has.
            @param bmp The image to use.
            @param offset The offset (in DIPs) from the top to draw the image.\n
                This can be useful for drawing the icon below the ascender line of the text
                (or even underneath the text).*/
        void SetTopImage(const wxBitmapBundle& bmp, const size_t offset = 0)
            {
            m_topImage = bmp;
            m_topImageOffset = offset;
            }
        /// @}

        /// @name Bounding Box Functions
        /// @brief Functions for customizing the label's bounding box.
        /// @{

        /** @returns The rectangle on the canvas where the label would fit in.
            @param dc An existing graphics context to measure the label on.
            @note This is a more optimal alternative to GetBoundingBox(), which doesn't have to
                create its own temporary @c wxDC.*/
        [[nodiscard]]
        wxRect GetBoundingBox(wxDC& dc) const final;
        /** @brief Bounds the label to be within the given rectangle.
            @param rect The rectangle to bound the label to.
            @param dc The DC to measure content with.
            @param parentScaling The parent's scaling (not used in this implementation).
            @note The scaling of the label will be adjusted to this box,
                and will also be anchored (length-wise if vertical, height-wise if horizontal)
                within this box.\n
                If the label isn't wide/tall enough to fill the bounding box precisely
                width- and length-wise, then it will be anchored.\n
                Call SetAnchoring() to control how it is anchored.
            @sa SetBoundingBoxToContentAdjustment().*/
        void SetBoundingBox(const wxRect& rect, wxDC& dc, [[maybe_unused]] const double parentScaling) final;

        /** @brief Moves the item by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final
            {
            SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove,yToMove));
            InvalidateCachedBoundingBox();
            }

        /** @brief Set how the bounding box passed to SetBoundingBox() is used.
            @details This behaviour is turned off by default (i.e., @c None),
                so that calls to SetBoundingBox() will explicitly set the size.\n
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
        void SetBoundingBoxToContentAdjustment(const LabelBoundingBoxContentAdjustment adjust) noexcept
            { m_adjustBoundingBoxToContent = adjust; }
        /// @returns How the bounding box passed to SetBoundingBox() is being applied.
        [[nodiscard]]
        LabelBoundingBoxContentAdjustment GetBoundingBoxToContentAdjustment() const noexcept
            { return m_adjustBoundingBoxToContent; }
        /// @}

        /// @name Font Functions
        /// @brief Helper functions for font selection and adjustments.
        /// @note To change or edit the font for a label, call
        ///     `GetGraphItemInfo().Font()` or `GetFont()`.
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
                setting its point size to this should also set its scaling to 1 (not the parent's).*/
        [[nodiscard]]
        static int CalcDiagonalFontSize(wxDC& dc,
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
                setting its point size to this should also set its scaling to @c 1.0
                (not the parent's)*/
        [[nodiscard]]
        static int CalcFontSizeToFitBoundingBox(wxDC& dc, const wxFont& ft,
                                                const wxRect& boundingBox,
                                                const wxString& text);
        /** @returns The first available font (face name) found from the given list,
                or the system default if none are found.
            @param possibleFontNames The list of font names to choose from.*/
        [[nodiscard]]
        static wxString GetFirstAvailableFont(const std::vector<wxString>& possibleFontNames);
        /** @returns The first available cursive font (face name) found on the system,
                or the system default if none are found.
            @note This function uses a list of known cursive fonts to search with.*/
        [[nodiscard]]
        static wxString GetFirstAvailableCursiveFont();
        /** @returns The first available monospace font (face name) found on the system,
                or the system default if none are found.
            @note This function uses a list of monospace fonts to search with.*/
        [[nodiscard]]
        static wxString GetFirstAvailableMonospaceFont();
        /// @brief Corrects issues with fonts such as bogus facenames and point sizes.
        /// @param theFont The font to review and correct.
        static void FixFont(wxFont& theFont);
        /// @}
    private:
        /** @brief Draws the line styling onto the background of the label.
            @param dc The dc to draw on.*/
        void DrawLabelStyling(wxDC& dc) const;
        /** @brief Draws the icons for the legend.
            @param dc The dc to draw on.*/
        void DrawLegendIcons(wxDC& dc) const;
        /// @returns The size that the left image will be if the provided height is given.
        /// @note This will maintain the image's aspect ratio and the calculated height
        ///     may be smaller than @c textHeight.
        /// @param textHeight The current height of the label.
        [[nodiscard]]
        wxSize CalcLeftImageSize(const wxCoord textHeight) const;
        /// @returns The size that the top image will be if the provided width is given.
        /// @note This will maintain the image's aspect ratio and the calculated width
        ///     may be smaller than @c textWidth.
        /// @param textWidth The current width of the label.
        [[nodiscard]]
        wxSize CalcTopImageSize(const wxCoord textWidth) const;
        /** @returns @c true if the given point is inside of the label.
            @param pt The point to check.*/
        [[nodiscard]]
        bool HitTest(const wxPoint pt, wxDC& dc) const final
            { return GetBoundingBox(dc).Contains(pt); }
        /** @brief Draws a vertical multi-line text string at the specified point,
             using the current text font, and the current text foreground and background colors.
            @param dc The device context to draw to.
            @param pt The point to draw the text.\n
                This coordinate refers to the bottom-left corner of the rectangle
                bounding the string.*/
        void DrawVerticalMultiLineText(wxDC& dc, wxPoint pt) const;
        /** @brief Draws a multi-line text string at the specified point, using the current
                 text font, and the current text foreground and background colors.
            @param dc The device context that the text is being drawn on.
            @param pt The point to draw the text.\n
                This coordinate refers to the top-left corner of the rectangle bounding the string.*/
        void DrawMultiLineText(wxDC& dc, wxPoint pt) const;
        /// @brief Figures out how many characters are in the longest line of text
        ///     (takes multiline labels into account).
        void CalcLongestLineLength();
        /** @brief Retrieves The physical size of label and its padding
                (including outlined bounding box if the pen is valid).
            @details Should only be called by GetBoundingBox(), which avoids calling this if
                the bounding box values are already cached. GetBoundingBox() will also take into
                account the (optional) minimum size of the label in conjunction with this.*/
        void GetSize(wxDC& dc, wxCoord& width, wxCoord& height) const;
        /// @returns The offset from the top if user-defined minimum size is being used it is
        ///  taller than the measured size.
        [[nodiscard]]
        wxCoord CalcPageVerticalOffset(const wxDC& dc) const;
        /// @returns The offset from the left if user-defined minimum size is being used it is
        ///     wider than the measured size.
        [[nodiscard]]
        wxCoord CalcPageHorizontalOffset(const wxDC& dc) const;

        double m_tiltAngle{ 0 };
        double m_spacingBetweenLines{ 1 };
        LabelBoundingBoxContentAdjustment m_adjustBoundingBoxToContent
            { LabelBoundingBoxContentAdjustment::ContentAdjustNone };
        size_t m_lineCount{ 0 };
        size_t m_longestLineLength{ 0 };
        std::set<size_t> m_linesIgnoringLeftMargin;
        BoxCorners m_boxCorners{ BoxCorners::Straight };
        wxBitmapBundle m_leftImage;
        wxBitmapBundle m_topImage;
        size_t m_topImageOffset{ 0 };
        };
    }

/** @}*/

#endif //__WISTERIA_CANVASLABEL_H__
