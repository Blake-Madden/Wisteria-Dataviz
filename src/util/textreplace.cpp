///////////////////////////////////////////////////////////////////////////////
// Name:        textreplace.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "textreplace.h"

namespace Wisteria
    {
    //----------------------------------------------------------------
    wxString TextReplace::operator()(const wxString& str) const
        {
        wxString strToAbbreviate{ str };
        // apply all the text replacements in the string
        for (const auto& replacement : m_replacements)
            {
            if (replacement.first && replacement.first->IsValid())
                {
                replacement.first->ReplaceAll(&strToAbbreviate, replacement.second);
                }
            }
        return strToAbbreviate;
        }

    //----------------------------------------------------------------
    AbbreviateEnglish::AbbreviateEnglish(const bool aggressive /*= false*/)
        {
        m_replacements =
            {
                // Group capturing is used to preserve mixed casing.
                // This way, you get "Services" -> "Svc.",
                // "SERVICES" -> "SVC.", and "services" -> "svc."
                { std::make_shared<wxRegEx>(L"(?i)\\b(math)(ematics)\\b"), L"\\1" },
                { std::make_shared<wxRegEx>(L"(?i)\\b(e)(ng)(inee)(r)(ing)\\b"), L"\\1\\2\\4." },
                { std::make_shared<wxRegEx>(L"(?i)\\b(m)(anu)(f)(acturin)(g)\\b"), L"\\1\\3\\5." },
                { std::make_shared<wxRegEx>(L"(?i)\\b(tech)(nology)\\b"), L"\\1" },
                { std::make_shared<wxRegEx>(L"(?i)\\b(tech)(nologies)\\b"), L"\\1" },
                { std::make_shared<wxRegEx>(L"(?i)\\b(s)(er)(v)(i)(c)(es)\\b"), L"\\1\\3\\5." },
                { std::make_shared<wxRegEx>(L"(?i)\\b(d)(ep)(ar)(t)(ment)\\b"), L"\\1\\2\\4." },
                { std::make_shared<wxRegEx>(L"(?i)\\b(h)(um)(anities)\\b"), L"\\1\\2." },
                { std::make_shared<wxRegEx>(L"(?i)\\b(c)(omm)(unication)\\b"), L"\\1\\2." },
                { std::make_shared<wxRegEx>(L"(?i)\\b(c)(omm)(unications)\\b"), L"\\1\\2." }
            };
        // if needing more extreme abbreviations
        if (aggressive)
            {
            AddReplacement(std::make_shared<wxRegEx>(L"(?i)\\b(sci)(ence)\\b"), L"\\1.");
            AddReplacement(std::make_shared<wxRegEx>(L"(?i)\\b(soc)(ial)\\b"), L"\\1.");
            AddReplacement(std::make_shared<wxRegEx>(L"(?i)\\b(pub)(lic)\\b"), L"\\1.");
            }
        }
    }
