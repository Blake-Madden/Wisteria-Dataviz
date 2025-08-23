/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef FORMAT_FORMULA_H
#define FORMAT_FORMULA_H

#include <wx/numformatter.h>
#include <wx/string.h>

/// @brief Class for formatting math formulas between U.S. and non-U.S. locales.
class FormulaFormat
    {
  public:
    /// @brief Converts a U.S.-formatted math expression into the current locale format.
    /// @param expression The U.S.-formatted formula to convert.
    /// @returns The formula in the current locale's format.
    static wxString FormatMathExpressionFromUS(const wxString& expression);
    /// @brief Converts a locale-formatted math expression into U.S. format.
    /// @param expression The locale-formatted formula to convert.
    /// @returns The formula in the U.S. format.
    static wxString FormatMathExpressionToUS(const wxString& expression);

    /// @returns The locale's list separator.
    static wchar_t GetListSeparator() noexcept
        {
        // a bit of a hack, but there is no standard way of getting this
        return (wxNumberFormatter::GetDecimalSeparator() == L'.') ? L',' : L';';
        }
    };

    /** @}*/

#endif // FORMAT_FORMULA_H
