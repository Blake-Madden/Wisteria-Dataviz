/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_COLOR_SCHEME_NAMES_H
#define WISTERIA_COLOR_SCHEME_NAMES_H

#include "colorbrewer.h"
#include <memory>
#include <utility>
#include <vector>
#include <wx/string.h>

namespace Wisteria::Colors::Schemes
    {
    /// @brief Central catalog of the library's named, built-in color schemes.
    /// @details Gives every named scheme (@c Dusk, @c EarthTones, @c Decade1980s, etc.) a
    ///     display name and a lowercase key, and converts between a scheme instance, its
    ///     key, and its position in the catalog (for populating a @c wxChoice/@c
    ///     wxComboBox, or for reading/writing a scheme name in project JSON).
    class ColorSchemeCatalog
        {
      public:
        /// @returns The catalog entries, in display order, as (display name, lowercase key)
        ///     pairs. Does not include a "Default"/"None" entry; callers that need one
        ///     add it themselves.
        [[nodiscard]]
        static const std::vector<std::pair<wxString, wxString>>& GetEntries();

        /// @returns The named color scheme for @p key (case-insensitive), or @c nullptr
        ///     when @p key is empty or unrecognized.
        [[nodiscard]]
        static std::shared_ptr<ColorScheme> FromKey(const wxString& key);

        /// @returns The lowercase key for @p scheme, identified by its concrete type, or
        ///     an empty string when @p scheme is @c nullptr or unrecognized.
        [[nodiscard]]
        static wxString ToKey(const std::shared_ptr<ColorScheme>& scheme);
        };
    } // namespace Wisteria::Colors::Schemes

/** @}*/

#endif // WISTERIA_COLOR_SCHEME_NAMES_H
