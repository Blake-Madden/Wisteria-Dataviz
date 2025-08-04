/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_CURRENCY_FORMAT_H
#define WISTERIA_CURRENCY_FORMAT_H

#include "wx/string.h"
#include <wx/numformatter.h>
#ifdef __UNIX__
    #include <monetary.h>
#endif

namespace Wisteria
    {
    /** @brief Formats a number to a currency string (in the current locale format).
        @param money The value to format.
        @param noTrailingZeroes @c true to remove fractional amounts if zero.
        @returns The monetary amount as a formatted string.
    */
    [[nodiscard]]
    inline wxString ToCurrency(double money, bool noTrailingZeroes = false)
        {
#ifdef __WXMSW__
        const wxString inputVal = wxString::FromCDouble(money);
        if (!inputVal.empty())
            {
            // see how much space we need
            const int outputBufferLength = ::GetCurrencyFormatEx(wxUILocale::GetCurrent().GetName(),
                                                                 0, inputVal, nullptr, nullptr, 0);
            if (outputBufferLength > 0)
                {
                auto currencyStr = std::make_unique<wchar_t[]>(outputBufferLength);
                if (::GetCurrencyFormatEx(wxUILocale::GetCurrent().GetName(), 0, inputVal, nullptr,
                                          currencyStr.get(), outputBufferLength) != 0)
                    {
                    wxString formattedMoney{ currencyStr.get() };
                    // output is always formatted with cents (even if zero), so we need
                    // to manually chop that off upon request
                    if (noTrailingZeroes)
                        {
                        wxNumberFormatter::RemoveTrailingZeroes(formattedMoney);
                        }
                    return formattedMoney;
                    }
                }
            }
#else
        char formattedBuffer[1024]{ 0 };
        /// @todo strfmon_l is preferred for later if we can ever get the locale_t object
        ///     from `wxUILocale::GetCurrent()`.
        if (strfmon(formattedBuffer, std::size(formattedBuffer), "%n", money) != -1)
            {
            wxString formattedMoney{ formattedBuffer };
            if (noTrailingZeroes)
                {
                wxNumberFormatter::RemoveTrailingZeroes(formattedMoney);
                }
            return formattedMoney;
            }
#endif
        return {};
        }
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_CURRENCY_FORMAT_H
