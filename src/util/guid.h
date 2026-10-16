/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_GUID_H
#define WISTERIA_GUID_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

namespace Wisteria
    {
    /// @brief Generates a random (version 4) GUID, as described in RFC 9562, section 5.4.
    /// @returns A GUID string in the form `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`
    ///     (uppercase hex). The `4` is the version and `y` is one of 8, 9, A, or B (the variant).
    [[nodiscard]]
    inline std::wstring GenerateGuid()
        {
        std::random_device randomDevice;
        std::array<uint8_t, 16> bytes{};
        for (auto& byte : bytes)
            {
            byte = static_cast<uint8_t>(randomDevice() & 0xFF);
            }
        // set the version (4) in the high nibble of byte 6
        bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0F) | 0x40);
        // set the variant (binary 10) in the top two bits of byte 8
        bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3F) | 0x80);

        // 8-4-4-4-12 hex digits
        constexpr std::wstring_view HEX_DIGITS{ L"0123456789ABCDEF" };
        std::wstring guid;
        guid.reserve(36);
        for (size_t idx = 0; idx < bytes.size(); ++idx)
            {
            if (idx == 4 || idx == 6 || idx == 8 || idx == 10)
                {
                guid += L'-';
                }
            guid += HEX_DIGITS[bytes[idx] >> 4];
            guid += HEX_DIGITS[bytes[idx] & 0x0F];
            }
        return guid;
        }
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_GUID_H
