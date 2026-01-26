///////////////////////////////////////////////////////////////////////////////
// Name:        formattedtextctrl_mac.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_FORMATTEDTEXTCTRL_MAC_H
#define WISTERIA_FORMATTEDTEXTCTRL_MAC_H

#ifdef __WXOSX__

    #include "../../base/canvas.h"
    #include <wx/gdicmn.h>
    #include <wx/string.h>

namespace Wisteria::UI
    {
    /** @brief Prints RTF content using native macOS printing.
        @param rtfContent The RTF content to print.
        @param paperSize The paper size in points (1/72 inch).
        @param orientation @c wxPORTRAIT or @c wxLANDSCAPE.
        @param header The header string (supports `@PN` for page number, `@PC` for page count).
        @param footer The footer string (supports `@PN` for page number, `@PC` for page count).
        @param watermark Watermark information to render across the pages when printing.*/
    void macOSPrintRTF(const wxString& rtfContent, const wxSize& paperSize, int orientation,
                       const wxString& header, const wxString& footer,
                       const Canvas::Watermark& watermark);
    } // namespace Wisteria::UI

#endif // __WXOSX__

#endif // WISTERIA_FORMATTEDTEXTCTRL_MAC_H
