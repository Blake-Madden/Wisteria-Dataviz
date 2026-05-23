///////////////////////////////////////////////////////////////////////////////
// Name:        colorlookup.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "colorlookup.h"
#include "colorbrewer.h"
#include "reportbuilder.h"

namespace Wisteria::Colors
    {
    //-------------------------------------------
    wxString ColorLookup::GetReadableColorName(const wxColour& color)
        {
        if (!color.IsOk())
            {
            return {};
            }
        // try to match against the Wisteria color catalog first...
        for (const auto& [name, colorId] : Wisteria::ReportBuilder::GetColorMap())
            {
            if (ColorBrewer::GetColor(colorId) == color)
                {
                return wxString{ name.data(), name.size() };
                }
            }
        // ...fall back to wxWidgets's named-color lookup, then to a hex string.
        const auto name = color.GetAsString(wxC2S_NAME);
        return name.empty() ? color.GetAsString(wxC2S_HTML_SYNTAX) : name;
        }
    } // namespace Wisteria::Colors
