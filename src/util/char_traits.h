/********************************************************************************
 * Copyright (c) 2005-2024 Blake Madden
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Blake Madden - initial implementation
 ********************************************************************************/

/** @addtogroup Utilities
    @brief Utility classes.
@{*/

#ifndef __INSENSITIVE_STRING_H__
#define __INSENSITIVE_STRING_H__

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <ios>
#include <string_view>

namespace string_util
    {
    /// @brief Case insensitive comparison for `std::basic_string<wchar_t>` strings.
    /// @private
    class case_insensitive_character_traits : public std::char_traits<wchar_t>
        {
      public:
        //--------------------------------------------------
        inline static bool eq_int_type(const int_type& i1, const int_type& i2) noexcept
            {
            return tolower(i1) == tolower(i2);
            }

        //--------------------------------------------------
        inline static bool eq(const char_type& first, const char_type& second) noexcept
            {
            return (tolower(first) == tolower(second));
            }

        //--------------------------------------------------
        inline static bool lt(const char_type& first, const char_type& second) noexcept
            {
            return (tolower(first) < tolower(second));
            }

        //--------------------------------------------------
        inline static char_type tolower(const char_type& ch) noexcept { return std::towlower(ch); }

        //--------------------------------------------------
        static int compare(const char_type* s1, const char_type* s2, size_t n) noexcept
            {
            assert(s1);
            assert(s2);
            if (s1 == nullptr)
                {
                return -1;
                }
            else if (s2 == nullptr)
                {
                return 1;
                }
            for (size_t i = 0; i < n; ++i)
                {
                if (!eq(s1[i], s2[i]))
                    {
                    return lt(s1[i], s2[i]) ? -1 : 1;
                    }
                }
            return 0;
            }

        //--------------------------------------------------
        static const char_type* find(const char_type* s1, size_t n, const char_type ch) noexcept
            {
            assert(s1);
            if (s1 == nullptr)
                {
                return nullptr;
                }
            for (size_t i = 0; i < n; ++i)
                {
                if (eq(s1[i], ch))
                    {
                    return s1 + i;
                    }
                }
            return nullptr;
            }

        //--------------------------------------------------
        static const char_type* find(const char_type* s1, size_t n1, const char_type* s2,
                                     size_t n2) noexcept
            {
            assert(n1 && n2);
            assert(s1);
            assert(s2);
            if (s1 == nullptr || s2 == nullptr || (n2 > n1))
                {
                return nullptr;
                }
            size_t j = 1;
            for (size_t i = 0; i < n1; i += j)
                {
                // if the first character of the substring matches then start comparing
                if (eq(s1[i], s2[0]))
                    {
                    // if only looking for one character then return
                    if (n2 == 1)
                        {
                        return s1 + i;
                        }
                    // already know the first chars match, so start at next one
                    for (j = 1; j < n2; ++j)
                        {
                        if (!eq(s1[i + j], s2[j]))
                            {
                            break;
                            }
                        }
                    // if every character matched then return it
                    if (n2 == j)
                        {
                        return s1 + i;
                        }
                    }
                }
            return nullptr;
            }
        };

    /// @brief Case-insensitive @c std::wstring.
    using case_insensitive_wstring = std::basic_string<wchar_t, case_insensitive_character_traits>;
    /// @brief Case-insensitive @c std::wstring_view.
    using case_insensitive_wstring_view =
        std::basic_string_view<wchar_t, case_insensitive_character_traits>;
    } // namespace string_util

/** @}*/

#endif //__INSENSITIVE_STRING_H__
