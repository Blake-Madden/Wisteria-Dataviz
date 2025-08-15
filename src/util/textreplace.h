/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __TEXTREPLACE_H__
#define __TEXTREPLACE_H__

#include <memory>
#include <vector>
#include <wx/regex.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria
    {
    /** @brief Text replacement helper.
        @details This class maintains a catalog of regular expressions to replace
            and their respective replacement values. These replacements can then
            be applied to a given string, returning the altered version.\n
            This can be useful abbreviating strings, as an example.*/
    class TextReplace
        {
      public:
        /** @brief Interface for applying replacements to a string.
            @param str The string to make replacements within.
            @returns The altered string.*/
        [[nodiscard]]
        wxString operator()(const wxString& str) const;

        /** @brief Adds a replacement item to the catalog.
            @param regExToReplace A regular expression to search for in text.
            @param replacement The text to replace any matches for @c regExToReplace.
            @warning For regular expressions, it is recommended to wrap words being
                replaced inside word-boundary anchors (@c \\b) to avoid unexpected results.*/
        void AddReplacement(const std::shared_ptr<wxRegEx>& regExToReplace,
                            const wxString& replacement)
            {
            if (regExToReplace && regExToReplace->IsValid())
                {
                m_replacements.emplace_back(regExToReplace, replacement);
                }
            else
                {
                wxLogWarning(L"Invalid regular expression passed to text replacement helper.");
                }
            }

      protected:
        /// @brief The text replacement map. This should be initialized with default values
        ///     in derived classes.
        std::vector<std::pair<std::shared_ptr<wxRegEx>, wxString>> m_replacements;
        };

    /** @brief Abbreviation class for English text.
        @details Includes common English abbreviations, but can also be expanded
            by calling AddReplacement().\n
            The following replacements are included:
            - Mathematics -> Math
            - Engineering -> Engr.
            - Manufacturing -> Mfg.
            - Technology -> Tech
            - Technologies -> Tech
            - Services -> Svc.
            - Department -> Dept.
            - Communication -> Comm.
            - Communications -> Comm.

            If aggressive, the following will also be included:
            - Science -> Sci.
            - Social -> Soc.
            - Public -> Pub.*/
    class AbbreviateEnglish final : public TextReplace
        {
      public:
        /// @brief Constructor.
        /// @param aggressive Set to @c true to use more aggressive abbreviations.
        explicit AbbreviateEnglish(bool aggressive = false);
        };
    } // namespace Wisteria

/** @}*/

#endif //__TEXTREPLACE_H__
