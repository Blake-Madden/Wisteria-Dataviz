/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_COLORLOOKUP_H
#define WISTERIA_COLORLOOKUP_H

#include <wx/colour.h>
#include <wx/string.h>

namespace Wisteria::Colors
    {
    /// @brief Reverse-lookup helpers for mapping @c wxColour values back to
    ///     human-readable names.
    class ColorLookup
        {
      public:
        /** @brief Returns a human-readable name for a color, suitable for
                accessibility descriptions (e.g., screen reader text).
            @details First tries to match @c color against the named colors in
                the Wisteria color catalog (via @c ReportBuilder::GetColorMap()).
                If that fails, falls back to wxWidgets' standard color name lookup
                (@c wxC2S_NAME) and finally the HTML hex representation
                (e.g., @c "#FFC0CB").
            @param color The color to describe.
            @returns The readable color name or hex string.*/
        [[nodiscard]]
        static wxString GetReadableColorName(const wxColour& color);
        };
    } // namespace Wisteria::Colors

/** @}*/

#endif // WISTERIA_COLORLOOKUP_H
