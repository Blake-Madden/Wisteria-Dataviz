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
        @note Call GetLabelDisplayInfo() to edit most of the appearance and layout functionality for a label.*/
    class Label final : public GraphItemBase
        {
        friend class Graphs::Graph2D;
        friend class Graphs::HeatMap;
        friend class Wisteria::Canvas;
        friend class GraphItems::Axis;
        friend class GraphItems::Points2D;
        friend class GraphItems::GraphItemBase;
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

        /** @brief Chop the string up so that it will fit within a bounding box.
            @param dc The device context to measure with.
            @param boundingBoxSize The size of the bounding box to fit the text into.
            @note If the bounding box isn't tall enough to fit the text, then the text
             will be truncated and have an ellipsis appended to it.*/
        void SplitTextToFitBoundingBox(wxDC& dc, const wxSize& boundingBoxSize);

        /** @brief Splits the string into multiline chunks, with each line being around the suggested length argument.
            @details String will be split on these delimiters: spaces and hyphens.
            @param suggestedLineLength The suggested length (character count) for each new line to be.*/
        void SplitTextToFitLength(const size_t suggestedLineLength);

        /** @brief Splits the text into lines containing only one character.
            @note It is recommended to use a fixed-width font (@c wxFONTFAMILY_TELETYPE) for the best appearance.
            @par Example
            @code
             // set the left title of a plot to be written downward
             // letter-by-letter
             thePlot->GetLeftYAxis().GetTitle().GetGraphItemInfo().
                Label(L"TIME").Orient(Orientation::Horizontal).
                Font(wxFontInfo(10).Family(wxFontFamily::wxFONTFAMILY_TELETYPE));
             thePlot->GetLeftYAxis().GetTitle().SplitTextByCharacter();
            @endcode*/
        void SplitTextByCharacter();

        /// @returns The minimum width needed for the left padding if including a legend. @sa SetLeftPadding().
        /// @warning This is only the pixel size, which is what SetLeftPadding() requires (i.e., no DPI or scaling applied).
        [[nodiscard]] static wxCoord GetMinLegendWidth() noexcept
            { return LegendIcon::GetIconWidth() + 2/* 1 pixel on each side of icon*/; }
        /** @returns The number of pixels between lines.
            @warning This will need to be scaled when being drawn or measured.*/
        [[nodiscard]] double GetLineSpacing() const noexcept
            { return m_spacingBetweenLines; }
        /** @brief Sets the number of pixels between lines (if label is multiline).
            @param spacing The number of pixels to space between lines.
            @note This is a pixel value that the framework will scale to the screen for you. The label's own scaling
             will also be applied when needed.*/
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
            @param parentScaling The parent's scaling (only used if IsUsingParentScalingOnBoudingAdjustment() is `true`).
            @note The scaling of the label will be adjusted (height-wise if vertical, length-wise if horizontal) to this box,
             and will also be anchored (length-wise if vertical, height-wise if horizontal) within this box.
             If the label isn't wide/tall enough to fill the bounding box, then it will be anchored.
             Call SetAnchoring() to control how it is anchored.*/
        void SetBoundingBox(const wxRect& rect, wxDC& dc, const double parentScaling) final;

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

        /// @name Font Functions
        /// @brief Helper functions for font selection and adjustments.
        /// @note To change or edit the font for a label, call @c GetGraphItemInfo().Font() or GetFont().
        /// @{

        /** @returns The best font size to fit a given string across an area diagonally.
            @param dc The device context to use for measuring.
            @param ft The font that will be used to measure with.
            @param boundingBox The bounding box to constrain the text within.
            @param angleInDegrees The angle that the string will be at.
            @param text The string being measured.
            @warning `boundingBoxSize` is assumed to be scaled from the parent already, so the font size returned will
             fit this area as-is. Because `Label`s adjust their font size based on scaling,
             setting a label's point size to this should also set its scaling to 1 (not the parents').*/
        [[nodiscard]] static int CalcDiagonalFontSize(wxDC& dc, const wxFont& ft, const wxRect& boundingBox,
            const double angleInDegrees, const wxString& text);
        /** @returns The font size that would fit for a given string within a given bounding box.
            @param dc The device context that the text is being drawn on.
            @param ft The font this is being drawn with.
            @param boundingBox The bounding box that the text should fit inside of.
            @param text The text that is being drawn.
            @warning `boundingBoxSize` is assumed to be scaled from the parent already, so the font size returned will
             fit this area as-is. Because `Label`s adjust their font size based on scaling,
             setting a label's point size to this should also set its scaling to 1 (not the parents')*/
        [[nodiscard]] static int CalcFontSizeToFitBoundingBox(wxDC& dc, const wxFont& ft,
            const wxRect& boundingBox, const wxString& text);
        /** @returns The first available font (face name) found from the given list, or the system default if none are found.
            @param possibleFontNames The list of font names to choose from.*/
        [[nodiscard]] static wxString GetFirstAvailableFont(const wxArrayString& possibleFontNames);
        /** @returns The first available cursive font (face name) found on the system, or the system default if none are found.
            @note This function uses a list of known cursive fonts to search with.*/
        [[nodiscard]] static wxString GetFirstAvailableCursiveFont();
        /// @brief Corrects issues with fonts such as bogus facenames and point sizes.
        /// @param theFont The font to review and correct.
        static void FixFont(wxFont& theFont);
        /// @}
    private:
        /** If this is a fixed object on a canvas, then when SetBoundingBox() is called this label's scaling (and font)
            will adjust accordingly to fit the box (the default behavior).
            Set this to `true` if you wish for the label to only be anchored within this box, but use the scaling
            of the parent canvas. This is useful if you have multiple labels that need to have a uniform font size.
            @param useParentScalining Whether to use the parent's scaling if SetBoundingBox() is called. */
        void UseParentScalingOnBoudingAdjustment(const bool useParentScalining) noexcept
            { m_useParentScalingOnRebounding = useParentScalining; }
        /// @returns Whether the parent's scaling is used for the font size when SetBoundingBox() is called.
        [[nodiscard]] bool IsUsingParentScalingOnBoudingAdjustment() const noexcept
            { return m_useParentScalingOnRebounding; }
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
                (GetText().length() > 0 && m_longestLineLength > 0), L"Longest line length in label was not calculated!");
            return m_longestLineLength;
            }
        /** @returns `true` if the given point is inside of the label.
            @param pt The point to check.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final
            { return GetBoundingBox(dc).Contains(pt); }
        /** @brief Draws a vertical multi-line text string at the specified point, using the current text font,
             and the current text foreground and background colours.
            @param dc The device context to draw to.
            @param pt The point to draw the text. This coordinate refers to the bottom-left corner of the rectangle bounding the string.
            @param leftOffset The padding to put on the left of each line.*/
        void DrawVerticalMultiLineText(wxDC& dc, wxPoint pt, const wxCoord leftOffset) const;
        /** @brief Draws a multi-line text string at the specified point, using the current text font,
             and the current text foreground and background colours.
            @param dc The device context that the text is being drawn on.
            @param pt The point to draw the text. This coordinate refers to the top-left corner of the rectangle bounding the string.
            @param leftOffset The padding to put on the left of each line.*/
        void DrawMultiLineText(wxDC& dc, wxPoint pt, wxCoord leftOffset) const;
        /// @brief Figures out how many characters are in the longest line of text (takes multiline labels into account).
        void CalcLongestLineLength();
        /** @brief Retrieves The physical size of label (including outlined bounding box if the pen is valid).
            @details Should only be called by GetBoundingBox(), which avoids calling this if the bounding box
             values are already cached. GetBoundingBox() will also take into account the (optional) minimum size
             of the label in conjunction with this.*/
        void GetSize(wxDC& dc, wxCoord& width, wxCoord& height) const;
        /// @returns The offset from the top if user-defined minimum size is being used it is taller
        ///  than the measured size.
        [[nodiscard]] wxCoord CalcPageVerticalOffset() const;
        /// @returns The offset from the left if user-defined minimum size is being used it is wider
        ///  than the measured size.
        [[nodiscard]] wxCoord CalcPageHorizontalOffset() const;

        double m_tiltAngle{ 0 };
        double m_spacingBetweenLines{ 1 };
        bool m_useParentScalingOnRebounding{ false };
        size_t m_lineCount{ 0 };
        size_t m_longestLineLength{ 0 };
        std::set<size_t> m_linesIgnoringLeftMargin;
        BoxCorners m_boxCorners{ BoxCorners::Straight };
        };
    }

/** @}*/

#endif //__WISTERIA_CANVASLABEL_H__
