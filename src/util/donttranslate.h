/********************************************************************************
 * Copyright (c) 2021-2025 Blake Madden
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

/** @addtogroup Internationalization
    @brief i18n classes.
@{*/

#ifndef __DONTTRANSLATE_H__
#define __DONTTRANSLATE_H__

#include <cstdint>
#include <type_traits>

// Determines whether T is string constant type.
/// @private
template<class T>
struct is_string_constant
    : std::bool_constant<std::is_same_v<T, const char*> || std::is_same_v<T, const wchar_t*> ||
                         std::is_same_v<T, const uint8_t*> || std::is_same_v<T, const char16_t*> ||
                         std::is_same_v<T, const char32_t*>>
    {
    };

// Helper for is_string_constant.
/// @private
template<class _Ty>
inline constexpr bool is_string_constant_v = is_string_constant<_Ty>::value;

/// @brief Explanations for why a string should not be available for translation.
enum class DTExplanation
    {
    DebugMessage,    /*!< Debugging/Tracing related string.*/
    LogMessage,      /*!< Log messages that aren't normally user facing.*/
    ProperNoun,      /*!< The name of a proper person, place, or thing that wouldn't
                          normally be translated.*/
    DirectQuote,     /*!< A direct quote (e.g., a German phrase) that should remain
                          in its original form.*/
    FilePath,        /*!< A filename or path.*/
    InternalKeyword, /*!< An internal keyword or constant.*/
    Command,         /*!< A command, such as "open" in a `ShellExecute()` call.*/
    SystemEntry,     /*!< A system entry, such as an entry in the Windows registry.*/
    FormatString,    /*!< A printf format string.*/
    Syntax,          /*!< Any sort of code or formula.*/
    Constant,        /*!< A constant being displayed that should never change.
                          For example, a number or math constant (e.g., "PI").*/
    NoExplanation,   /*!< No explanation.*/
    FontName         /*!< A font name.*/
    };

/** @brief "Don't Translate." Simply expands a string in place at compile time,
        while communicating to developers that is not meant to be translated.

        This is useful for explicitly stating that a string is not meant for localization.

        In essence, this is the opposite of the `_()` macro from the GNU *gettext* library
        that marks a string as translatable.
    @param str The string.
    @param explanation An optional type of explanation for why this string should not
        be available for translation.
    @param explanationMessage An optional message to add explaining why this shouldn't
        be translated. This is a useful alternative to wrapping comments around the code.
    @returns The same string.
    @note This works with `char`, `uint8_t`, `char16_t`, `char32_t`, and `wchar_t`
        type string constants.
    @sa _DT().
    @par Example
    @code
        const std::string fileName = "C:\\data\\logreport.txt";

        // "open " should not be translated, it's part of a command line
        auto command = DONTTRANSLATE("open ") + fileName;
        // expands to "open C:\\data\\logreport.txt"

        // a more descriptive approach
        auto command2 = DONTTRANSLATE("open ", DTExplanation::Command) + fileName;
        // also expands to "open C:\\data\\logreport.txt"

        // an even more descriptive approach
        auto command3 = DONTTRANSLATE("open ",
                                      DTExplanation::Command,
                                      "This is part of a command line, "
                                      "don't expose for translation!") +
                        fileName;
        // also expands to "open C:\\data\\logreport.txt"

        // a shorthand, _DT(), is also available
        auto command = _DT("open ") + fileName;
    @endcode*/
template<typename T, std::enable_if_t<is_string_constant_v<T>, bool> = true>
inline constexpr auto
DONTTRANSLATE(T str,
              [[maybe_unused]] const DTExplanation explanation = DTExplanation::NoExplanation,
              [[maybe_unused]] T explanationMessage = nullptr)
    {
    return str;
    }

/** @brief A shorthand alias for DONTTRANSLATE().
    @param str The string.
    @param explanation An optional type of explanation for why this string
        should not be available for translation.
    @param explanationMessage An optional message to add explaining why this
        should not be translated.
    @returns The same string.*/
template<typename T, std::enable_if_t<is_string_constant_v<T>, bool> = true>
inline constexpr auto
_DT(T str, [[maybe_unused]] const DTExplanation explanation = DTExplanation::NoExplanation,
    [[maybe_unused]] T explanationMessage = nullptr)
    {
    return str;
    }

/** @}*/

#endif //__DONTTRANSLATE_H__
