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

namespace Wisteria
    {
    [[nodiscard]]
    inline wxString ToCurrency(double val, bool noTrailingZeroes = false)
        {
#ifdef __WXMSW__
        const wxString inputVal = wxString::FromCDouble(val);
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
                    wxString result{ currencyStr.get() };
                    // output is always formatted with cents (even if zero), so we need
                    // to manually chop that off as best we can
                    if (noTrailingZeroes)
                        {
                        /// @todo use wxNumberFormatter::RemoveTrailingZeroes if ever public
                        const auto decimalPosition = result.rfind(
                            wxString{ wxNumberFormatter::GetDecimalSeparator() }.append("00"));
                        if (decimalPosition != wxString::npos)
                            {
                            result.erase(decimalPosition, 3);
                            }
                        }
                    return result;
                    }
                }
            }
#else
        return wxNumberFormatter::ToString(val, 2,
                                           wxNumberFormatter::Style::Style_WithThousandsSep |
                                               wxNumberFormatter::Style::Style_NoTrailingZeroes);
#endif
        return {};
        }
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_CURRENCY_FORMAT_H
