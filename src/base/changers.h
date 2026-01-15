/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_CHANGERS_H
#define WISTERIA_CHANGERS_H

#include <wx/wx.h>

namespace Wisteria
    {
    /// @brief Same as @c wxDCPenChanger, except that it won't
    ///     actually change the pens if they are the same.\n
    ///     This is an optimization to prevent unnecessary swaps.
    class DCPenChangerIfDifferent
        {
      public:
        /** @brief Constructor.
            @param dc The drawing DC.
            @param pen The pen to (possibly) switch to.*/
        DCPenChangerIfDifferent(wxDC& dc, const wxPen& pen)
            : m_dc(dc), m_penOld(dc.GetPen()), m_swapPens(pen.IsOk() && (pen != dc.GetPen()))
            {
            if (m_swapPens)
                {
                m_dc.SetPen(pen);
                }
            }

        /// @private
        DCPenChangerIfDifferent(const DCPenChangerIfDifferent&) = delete;
        /// @private
        DCPenChangerIfDifferent& operator=(const DCPenChangerIfDifferent&) = delete;

        /// @private
        ~DCPenChangerIfDifferent() noexcept
            {
            if (m_swapPens)
                {
                m_dc.SetPen(m_penOld);
                }
            }

      private:
        wxDC& m_dc;
        wxPen m_penOld;
        bool m_swapPens{ false };
        };

    /// @brief Same as @c wxDCBrushChanger, except that it won't
    ///     actually change the brushes if they are the same.\n
    ///     This is an optimization to prevent unnecessary swaps.
    class DCBrushChangerIfDifferent
        {
      public:
        /** @brief Constructor.
            @param dc The drawing DC.
            @param brush The brush to (possibly) switch to.*/
        DCBrushChangerIfDifferent(wxDC& dc, const wxBrush& brush)
            : m_dc(dc), m_brushOld(dc.GetBrush()),
              m_swapBrushes(brush.IsOk() && (brush != dc.GetBrush()))
            {
            if (m_swapBrushes)
                {
                m_dc.SetBrush(brush);
                }
            }

        /// @private
        DCBrushChangerIfDifferent(const DCBrushChangerIfDifferent&) = delete;
        /// @private
        DCBrushChangerIfDifferent& operator=(const DCBrushChangerIfDifferent&) = delete;

        /// @private
        ~DCBrushChangerIfDifferent() noexcept
            {
            if (m_swapBrushes)
                {
                m_dc.SetBrush(m_brushOld);
                }
            }

      private:
        wxDC& m_dc;
        wxBrush m_brushOld;
        bool m_swapBrushes{ false };
        };

    /// @brief Same as @c wxDCFontChanger, except that it won't
    ///     actually change the fonts if they are the same.\n
    ///     This is an optimization to prevent unnecessary swaps.
    class DCFontChangerIfDifferent
        {
      public:
        /** @brief Constructor.
            @param dc The drawing DC.
            @param font The font to (possibly) switch to.*/
        DCFontChangerIfDifferent(wxDC& dc, const wxFont& font)
            : m_dc(dc), m_fontOld(dc.GetFont()), m_swapFonts(font.IsOk() && (font != dc.GetFont()))
            {
            if (m_swapFonts)
                {
                m_dc.SetFont(font);
                }
            }

        /// @private
        DCFontChangerIfDifferent(const DCFontChangerIfDifferent&) = delete;
        /// @private
        DCFontChangerIfDifferent& operator=(const DCFontChangerIfDifferent&) = delete;

        /// @private
        ~DCFontChangerIfDifferent() noexcept
            {
            if (m_swapFonts)
                {
                m_dc.SetFont(m_fontOld);
                }
            }

      private:
        wxDC& m_dc;
        wxFont m_fontOld;
        bool m_swapFonts{ false };
        };

    /// @brief Same as @c wxDCTextColourChanger, except that it won't
    ///     actually change the text colors if they are the same.\n
    ///     This is an optimization to prevent unnecessary swaps.
    class DCTextColourChangerIfDifferent
        {
      public:
        /** @brief Constructor.
            @param dc The drawing DC.
            @param col The text color to (possibly) switch to.*/
        DCTextColourChangerIfDifferent(wxDC& dc, const wxColour& col)
            : m_dc(dc), m_colFgOld(dc.GetTextForeground()),
              m_swapColors(col.IsOk() && (col != dc.GetTextForeground()))
            {
            if (m_swapColors)
                {
                m_dc.SetTextForeground(col);
                }
            }

        /// @private
        DCTextColourChangerIfDifferent(const DCTextColourChangerIfDifferent&) = delete;
        /// @private
        DCTextColourChangerIfDifferent& operator=(const DCTextColourChangerIfDifferent&) = delete;

        /// @private
        ~DCTextColourChangerIfDifferent() noexcept
            {
            if (m_swapColors)
                {
                m_dc.SetTextForeground(m_colFgOld);
                }
            }

      private:
        wxDC& m_dc;
        wxColour m_colFgOld;
        bool m_swapColors{ false };
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_CHANGERS_H
