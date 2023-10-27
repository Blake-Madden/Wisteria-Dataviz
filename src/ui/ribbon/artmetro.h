/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Peter Cawley, "iwbnwif", Blake Madden
    @authors Peter Cawley (wxRibbon), "iwbnwif" (wxRibbonMetroArtProvider),
     Blake Madden (modernized code, minor fixes)
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the wxWindows license.

    SPDX-License-Identifier: wxWindows
@{*/

/*
Adaptation of ribbon art provider by user "iwbnwif" from:

https://forums.wxwidgets.org/viewtopic.php?f=21&t=37348&p=152217&hilit=art_metro#p152217
*/

/// Original art provider source code's copyright that was the basis for iwbnwif's code:

///////////////////////////////////////////////////////////////////////////////
// Name:        wx/ribbon/art.h
// Purpose:     Art providers for ribbon-bar-style interface
// Author:      Peter Cawley
// Modified by:
// Created:     2009-05-25
// RCS-ID:      $Id$
// Copyright:   (C) Peter Cawley
// License:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

#ifndef __RIBBON_METRO_ART_H__
#define __RIBBON_METRO_ART_H__

#include <wx/defs.h>
#include <wx/ribbon/art.h>
#include <wx/aui/aui.h>
#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/font.h>
#include <wx/pen.h>
#include <wx/bitmap.h>

/// @private
class WXDLLIMPEXP_FWD_CORE wxDC;
/// @private
class WXDLLIMPEXP_FWD_CORE wxWindow;

namespace Wisteria::UI
    {
    /// @brief Applies a color to an AUI toolbar.
    class ThemedAuiToolbarArt : public wxAuiGenericToolBarArt
        {
    public:
        /// @brief Sets the background color of the toolbar.
        /// @param color The background color.
        void SetThemeColor(const wxColour& color)
            {
            if (color.IsOk())
                { m_baseColour = color; }
            }
        };

    /// @brief Ribbon art provider that emulates the Windows 8 "metro" look.
    class RibbonMetroArtProvider : public wxRibbonMSWArtProvider
        {
    public:
        /** @brief Constructor.
            @param set_colour_scheme @c true to set the color scheme.*/
        RibbonMetroArtProvider(bool set_colour_scheme = true);
        /** @brief Sets provided flags.
            @param flags The flags to set.*/
        void SetFlags(long flags) final;

        /** @brief Gets the colour of the tabs.
            @param primary The color of the active tab and its tool area.
            @param secondary The color of the background (including inactive tabs).
            @param tertiary The background color of hovered buttons and non-active tabs.*/
        void GetColourScheme(wxColour* primary,
                             wxColour* secondary,
                             wxColour* tertiary) const final;

        /** @brief Sets the colour of the tabs.
            @param primary The color of the active tab and its tool area.
            @param secondary The color of the background (including inactive tabs).
            @param tertiary The background color of hovered buttons and non-active tabs.*/
        void SetColourScheme(const wxColour& primary,
                             const wxColour& secondary,
                             const wxColour& tertiary) final;
    private:
        void DrawTabCtrlBackground(
                            wxDC& dc,
                            [[maybe_unused]] wxWindow* wnd,
                            const wxRect& rect) final;

        void DrawTab(wxDC& dc,
                     [[maybe_unused]] wxWindow* wnd,
                     const wxRibbonPageTabInfo& tab) final;

        void DrawPageBackground(
                            wxDC& dc,
                            [[maybe_unused]] wxWindow* wnd,
                            const wxRect& rect) final;

        void DrawPanelBackground(
                            wxDC& dc,
                            wxRibbonPanel* wnd,
                            const wxRect& rect) final;

        void DrawMinimisedPanel(
                            wxDC& dc,
                            wxRibbonPanel* wnd,
                            const wxRect& rect,
                            wxBitmap& bitmap) final;

        void DrawButtonBarBackground(
                            wxDC& dc,
                            wxWindow* wnd,
                            const wxRect& rect) final;

        void DrawButtonBarButton(
                            wxDC& dc,
                            [[maybe_unused]] wxWindow* wnd,
                            const wxRect& rect,
                            wxRibbonButtonKind kind,
                            long state,
                            const wxString& label,
                            const wxBitmap& bitmap_large,
                            const wxBitmap& bitmap_small) final;

        void DrawToolBarBackground(
                            wxDC& dc,
                            wxWindow* wnd,
                            const wxRect& rect) final;

        void DrawToolGroupBackground(
                            wxDC& dc,
                            [[maybe_unused]] wxWindow* wnd,
                            const wxRect& rect) final;

        void DrawTool(
                    wxDC& dc,
                    [[maybe_unused]] wxWindow* wnd,
                    const wxRect& rect,
                    const wxBitmap& bitmap,
                    wxRibbonButtonKind kind,
                    long state) final;

        void DrawToggleButton(
                            wxDC& dc,
                            wxRibbonBar* wnd,
                            const wxRect& rect,
                            wxRibbonDisplayMode mode) final;

        void DrawHelpButton(
                    wxDC& dc,
                    wxRibbonBar* wnd,
                    const wxRect& rect) final;

        void DrawPartialPageBackground(
            wxDC& dc, wxWindow* wnd, const wxRect& rect,
            bool allow_hovered = true);
        void DrawPartialPageBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect,
             wxRibbonPage* page, wxPoint offset, [[maybe_unused]] bool hovered = false);
        void DrawPanelBorder(wxDC& dc, const wxRect& rect, [[maybe_unused]] wxPen& primary_colour,
            [[maybe_unused]] wxPen& secondary_colour);

        void DrawButtonBarButtonForeground(
                            wxDC& dc,
                            const wxRect& rect,
                            wxRibbonButtonKind kind,
                            long state,
                            const wxString& label,
                            const wxBitmap& bitmap_large,
                            const wxBitmap& bitmap_small);
        void DrawMinimisedPanelCommon(
                            wxDC& dc,
                            wxRibbonPanel* wnd,
                            const wxRect& rect,
                            wxRect* preview_rect);
        };
    }

/** @}*/

#endif // __RIBBON_METRO_ART_H__
