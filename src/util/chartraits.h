/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __INSENSITIVE_STRING_H__
#define __INSENSITIVE_STRING_H__

#include <cstring>
#include <string_view>
#include <cwchar>
#include <cwctype>
#include <cstdio>
#include <ios>
#include <cassert>

namespace string_util
    {
    /// @brief Case insensitive comparison for `std::basic_string<wchar_t>` strings.
    /// @private
    class case_insensitive_character_traits
        {
    public:
        using char_type = wchar_t;
        using int_type = wint_t;
        using off_type = std::streamoff;
        using pos_type = std::streampos;
        using state_type = std::mbstate_t;

        inline static bool eq_int_type(const int_type& i1, const int_type& i2) noexcept
            { return tolower(i1) == tolower(i2); }
        static constexpr int_type eof() noexcept
            { return static_cast<int_type>(EOF); }
        static constexpr int_type not_eof(const int_type& i) noexcept
            {
            // EOF is negative, so 0 != EOF
            return (i == static_cast<int_type>(EOF)) ? 0 : 1;
            }
        static constexpr char_type to_char_type(const int_type& i) noexcept
            { return static_cast<char_type>(i); }
        static constexpr int_type to_int_type(const char_type& c) noexcept
            { return static_cast<unsigned char>(c); }

        inline static size_t length(const char_type* s) noexcept
            { return std::wcslen(s); }

        inline static void assign(char_type& dst, const char_type src) noexcept
            { dst = src; }
        inline static char_type* assign(char_type* dst, size_t n, char_type c) noexcept
            { return std::wmemset(dst, c, n); }

        inline static char_type* move(char_type* dst, const char_type* src, size_t n) noexcept
            { return std::wmemmove(dst, src, n); }
        inline static char_type* copy(char_type* strDest, const char_type* strSource, size_t count) noexcept
            { return std::wcsncpy(strDest, strSource, count); }

        inline static bool eq(const char_type& first, const char_type& second) noexcept
            { return (tolower(first) == tolower(second) ); }

        inline static bool lt(const char_type& first, const char_type& second) noexcept
            { return (tolower(first) < tolower(second) ); }

        inline static char_type tolower(const char_type& ch) noexcept
            { return std::towlower(ch); }

        static int compare(const char_type* s1, const char_type* s2, size_t n) noexcept
            {
            assert(s1); assert(s2);
            if (s1 == nullptr)
              { return -1; }
            else if (s2 == nullptr)
              { return 1; }
            for (size_t i = 0; i < n; ++i)
              {
              if (!eq(s1[i], s2[i]) )
                  {
                  return lt(s1[i], s2[i]) ? -1 : 1;
                  }
              }
            return 0;
            }

        static const char_type* find(const char_type* s1, size_t n,
                                     const char_type ch) noexcept
              {
              assert(s1);
              if (s1 == nullptr)
                  { return nullptr; }
              for (size_t i = 0; i < n; ++i)
                  {
                  if (eq(s1[i], ch) )
                      {
                      return s1+i;
                      }
                  }
              return nullptr;
              }

        static const char_type* find(const char_type* s1, size_t n1,
                                     const char_type* s2, size_t n2) noexcept
              {
              assert(n1 && n2);
              assert(s1); assert(s2);
              if (s1 == nullptr || s2 == nullptr || (n2 > n1))
                  { return nullptr; }
              size_t j = 1;
              for (size_t i = 0; i < n1; i+=j)
                  {
                  // if the first character of the substring matches then start comparing
                  if (eq(s1[i], s2[0]) )
                      {
                      // if only looking for one character then return
                      if (n2 == 1)
                          {
                          return s1+i;
                          }
                      // already know the first chars match, so start at next one
                      for (j = 1; j < n2; ++j)
                          {
                          if (!eq(s1[i+j], s2[j]) )
                              {
                              break;
                              }
                          }
                      // if every character matched then return it
                      if (n2 == j)
                          {
                          return s1+i;
                          }
                      }
                  }
              return nullptr;
              }
        };

    /// @brief Case-insensitive @c std::wstring.
    using case_insensitive_wstring = std::basic_string<wchar_t, case_insensitive_character_traits>;
    using case_insensitive_wstring_view = std::basic_string_view<wchar_t, case_insensitive_character_traits>;
    }

/** @}*/

#endif //__INSENSITIVE_STRING_H__
