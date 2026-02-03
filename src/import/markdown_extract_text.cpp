///////////////////////////////////////////////////////////////////////////////
// Name:        markdown_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "markdown_extract_text.h"
#include <map>
#include <set>

// LaTeX command -> Unicode character lookup table.
// Only simple commands that map to a single Unicode character are included.
// clang-format off
static const std::map<std::wstring_view, wchar_t> LATEX_COMMAND_MAP
    {
    // Greek letters (lowercase)
    { L"alpha",    L'\x03B1' },  // α
    { L"beta",     L'\x03B2' },  // β
    { L"gamma",    L'\x03B3' },  // γ
    { L"delta",    L'\x03B4' },  // δ
    { L"epsilon",  L'\x03B5' },  // ε
    { L"zeta",     L'\x03B6' },  // ζ
    { L"eta",      L'\x03B7' },  // η
    { L"theta",    L'\x03B8' },  // θ
    { L"iota",     L'\x03B9' },  // ι
    { L"kappa",    L'\x03BA' },  // κ
    { L"lambda",   L'\x03BB' },  // λ
    { L"mu",       L'\x03BC' },  // μ
    { L"nu",       L'\x03BD' },  // ν
    { L"xi",       L'\x03BE' },  // ξ
    { L"pi",       L'\x03C0' },  // π
    { L"rho",      L'\x03C1' },  // ρ
    { L"sigma",    L'\x03C3' },  // σ
    { L"tau",      L'\x03C4' },  // τ
    { L"upsilon",  L'\x03C5' },  // υ
    { L"phi",      L'\x03C6' },  // φ
    { L"chi",      L'\x03C7' },  // χ
    { L"psi",      L'\x03C8' },  // ψ
    { L"omega",    L'\x03C9' },  // ω
    // Greek letters (uppercase)
    { L"Gamma",    L'\x0393' },  // Γ
    { L"Delta",    L'\x0394' },  // Δ
    { L"Theta",    L'\x0398' },  // Θ
    { L"Lambda",   L'\x039B' },  // Λ
    { L"Xi",       L'\x039E' },  // Ξ
    { L"Pi",       L'\x03A0' },  // Π
    { L"Sigma",    L'\x03A3' },  // Σ
    { L"Phi",      L'\x03A6' },  // Φ
    { L"Psi",      L'\x03A8' },  // Ψ
    { L"Omega",    L'\x03A9' },  // Ω
    // Relational operators
    { L"leq",      L'\x2264' },  // ≤ (less than or equal)
    { L"geq",      L'\x2265' },  // ≥ (greater than or equal)
    { L"neq",      L'\x2260' },  // ≠ (not equal)
    { L"approx",   L'\x2248' },  // ≈ (approximately equal)
    { L"equiv",    L'\x2261' },  // ≡ (identical/congruent)
    { L"sim",      L'\x223C' },  // ∼ (similar to)
    { L"propto",   L'\x221D' },  // ∝ (proportional to)
    { L"prec",     L'\x227A' },  // ≺ (precedes)
    { L"succ",     L'\x227B' },  // ≻ (succeeds)
    { L"ll",       L'\x226A' },  // ≪ (much less than)
    { L"gg",       L'\x226B' },  // ≫ (much greater than)
    // Set/logic operators
    { L"in",       L'\x2208' },  // ∈ (element of)
    { L"notin",    L'\x2209' },  // ∉ (not element of)
    { L"subset",   L'\x2282' },  // ⊂ (subset)
    { L"supset",   L'\x2283' },  // ⊃ (superset)
    { L"subseteq", L'\x2286' },  // ⊆ (subset or equal)
    { L"supseteq", L'\x2287' },  // ⊇ (superset or equal)
    { L"cup",      L'\x222A' },  // ∪ (union)
    { L"cap",      L'\x2229' },  // ∩ (intersection)
    { L"emptyset", L'\x2205' },  // ∅ (empty set)
    { L"forall",   L'\x2200' },  // ∀ (for all)
    { L"exists",   L'\x2203' },  // ∃ (there exists)
    { L"neg",      L'\x00AC' },  // ¬ (logical not)
    { L"land",     L'\x2227' },  // ∧ (logical and)
    { L"lor",      L'\x2228' },  // ∨ (logical or)
    // Arrows
    { L"to",           L'\x2192' },  // → (right arrow)
    { L"rightarrow",   L'\x2192' },  // → (right arrow)
    { L"leftarrow",    L'\x2190' },  // ← (left arrow)
    { L"leftrightarrow", L'\x2194' }, // ↔ (left-right arrow)
    { L"Rightarrow",   L'\x21D2' },  // ⇒ (double right arrow / implies)
    { L"Leftarrow",    L'\x21D0' },  // ⇐ (double left arrow)
    { L"Leftrightarrow", L'\x21D4' }, // ⇔ (double left-right arrow / iff)
    { L"uparrow",      L'\x2191' },  // ↑ (up arrow)
    { L"downarrow",    L'\x2193' },  // ↓ (down arrow)
    { L"mapsto",       L'\x21A6' },  // ↦ (maps to)
    // Large operators / calculus
    { L"sum",      L'\x2211' },  // ∑ (summation)
    { L"prod",     L'\x220F' },  // ∏ (product)
    { L"int",      L'\x222B' },  // ∫ (integral)
    { L"oint",     L'\x222E' },  // ∮ (contour integral)
    { L"partial",  L'\x2202' },  // ∂ (partial derivative)
    { L"nabla",    L'\x2207' },  // ∇ (nabla/del)
    // Miscellaneous math symbols
    { L"infty",    L'\x221E' },  // ∞ (infinity)
    { L"pm",       L'\x00B1' },  // ± (plus-minus)
    { L"mp",       L'\x2213' },  // ∓ (minus-plus)
    { L"times",    L'\x00D7' },  // × (multiplication)
    { L"div",      L'\x00F7' },  // ÷ (division)
    { L"cdot",     L'\x00B7' },  // · (centered dot)
    { L"star",     L'\x22C6' },  // ⋆ (star operator)
    { L"circ",     L'\x2218' },  // ∘ (ring operator / composition)
    { L"bullet",   L'\x2022' },  // • (bullet)
    { L"sqrt",     L'\x221A' },  // √ (square root)
    { L"angle",    L'\x2220' },  // ∠ (angle)
    { L"perp",     L'\x22A5' },  // ⊥ (perpendicular)
    { L"parallel", L'\x2225' },  // ∥ (parallel)
    { L"therefore", L'\x2234' }, // ∴ (therefore)
    { L"because",  L'\x2235' },  // ∵ (because)
    { L"ldots",    L'\x2026' },  // … (ellipsis)
    { L"cdots",    L'\x22EF' },  // ⋯ (centered ellipsis)
    { L"prime",    L'\x2032' },  // ′ (prime)
    { L"hbar",     L'\x210F' },  // ℏ (reduced Planck constant)
    { L"ell",      L'\x2113' },  // ℓ (script small l)
    { L"Re",       L'\x211C' },  // ℜ (real part)
    { L"Im",       L'\x2111' },  // ℑ (imaginary part)
    { L"aleph",    L'\x2135' },  // ℵ (aleph)
    { L"wp",       L'\x2118' },  // ℘ (Weierstrass p)
    { L"dagger",   L'\x2020' },  // † (dagger)
    { L"ddagger",  L'\x2021' },  // ‡ (double dagger)
    };
// clang-format on

//--------------------------------------------
void lily_of_the_valley::markdown_extract_text::transform_equation(std::wstring& equation)
    {
    size_t pos{ 0 };
    while (pos < equation.length())
        {
        if (equation[pos] == L'\\')
            {
            // find the end of the command name (letters only)
            const size_t cmdStart = pos + 1;
            size_t cmdEnd = cmdStart;
            while (cmdEnd < equation.length() && std::iswalpha(equation[cmdEnd]) != 0)
                {
                ++cmdEnd;
                }
            if (cmdEnd > cmdStart)
                {
                const std::wstring_view cmd{ equation.data() + cmdStart, cmdEnd - cmdStart };
                const auto it = LATEX_COMMAND_MAP.find(cmd);
                if (it != LATEX_COMMAND_MAP.cend())
                    {
                    // replace \command with the Unicode character
                    equation.replace(pos, cmdEnd - pos, 1, it->second);
                    // advance past the inserted character
                    ++pos;
                    continue;
                    }
                }
            }
        ++pos;
        }
    }

//--------------------------------------------
bool lily_of_the_valley::markdown_extract_text::parse_code_block(const bool isEscaping,
                                                                 const wchar_t* currentEndSentinel,
                                                                 const wchar_t*& currentStart,
                                                                 wchar_t& previousChar,
                                                                 bool& headerMode)

    {
    // fenced section
    if (!isEscaping && std::wcsncmp(currentStart, L"```", 3) == 0)
        {
        std::advance(currentStart, 3);
        const auto* endOfTag = std::wcsstr(currentStart, L"```");
        if (endOfTag == nullptr)
            {
            log_message(L"Bad fenced code block in markdown file.");
            return false;
            }
        bool isMultiline{ false };
        const auto* scanAhead{ currentStart };
        while (scanAhead < endOfTag)
            {
            if (*scanAhead == L'\r' || *scanAhead == L'\n')
                {
                isMultiline = true;
                break;
                }
            ++scanAhead;
            }
        // if Quarto and this block is not getting echoed, then don't include
        // it in the parsed text
        if (const auto* const includeFalse = std::wcsstr(currentStart, L"#| include: false");
            includeFalse != nullptr && includeFalse < endOfTag)
            {
            currentStart = std::next(endOfTag, 3);
            }
        else if (const auto* const echoFalse = std::wcsstr(currentStart, L"#| echo: false");
                 echoFalse != nullptr && echoFalse < endOfTag)
            {
            currentStart = std::next(endOfTag, 3);
            }
        else
            {
            bool pastFirstLine{ false };
            // tab over each line inside the code block
            while (currentStart < endOfTag)
                {
                if (*currentStart == L'\r' || *currentStart == L'\n')
                    {
                    while (currentStart < endOfTag &&
                           (*currentStart == L'\r' || *currentStart == L'\n'))
                        {
                        add_character(*currentStart);
                        std::advance(currentStart, 1);
                        }
                    add_character(L'\t');
                    pastFirstLine = true;
                    continue;
                    }
                if (!isMultiline || pastFirstLine)
                    {
                    // step over line if Quarto code block directive
                    if (std::wcsncmp(currentStart, L"#| ", 3) == 0)
                        {
                        while (currentStart < endOfTag &&
                               !(*currentStart == L'\r' || *currentStart == L'\n'))
                            {
                            std::advance(currentStart, 1);
                            }
                        continue;
                        }
                    add_character(*currentStart);
                    }
                std::advance(currentStart, 1);
                }
            currentStart = std::next(endOfTag, 3);
            }
        // if code block is not inline, then force a line break later after it
        if (currentStart < currentEndSentinel &&
            ((*currentStart == L'\r' || *currentStart == L'\n')))
            {
            headerMode = true;
            }
        return true;
        }
    // verbatim (inline) code
    if (!isEscaping)
        {
        // RMarkdown code should be left as-is, but with the 'r' prefix removed
        // (or processed for known functions)
        if (std::wcsncmp(currentStart, L"`r keys(", 8) == 0)
            {
            std::advance(currentStart, 8);
            if (currentStart + 1 < currentEndSentinel &&
                (currentStart[1] == L'\'' || currentStart[1] == L'"'))
                {
                std::advance(currentStart, 1);
                }
            if (*currentStart == L'\'' || *currentStart == L'"')
                {
                const auto quoteChar{ *currentStart };
                std::advance(currentStart, 1);
                const auto* endOfTag = string_util::find_unescaped_char_same_line_n(
                    currentStart, quoteChar, std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                    {
                    log_message(L"Bad 'r keys' code block in markdown file.");
                    return false;
                    }
                [[maybe_unused]]
                const auto* retval = m_subParser->operator()(
                    { currentStart, static_cast<size_t>(std::distance(currentStart, endOfTag)) });
                add_character(L'"');
                add_characters(
                    { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                add_character(L'"');
                if (m_subParser->get_filtered_text_length() != 0U)
                    {
                    previousChar =
                        m_subParser
                            ->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                    }
                endOfTag = string_util::find_unescaped_char_same_line_n(
                    currentStart, L'`', std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                    {
                    log_message(L"Bad 'r keys' code block in markdown file.");
                    return false;
                    }
                currentStart = ++endOfTag;
                }
            return true;
            }
        if (std::wcsncmp(currentStart, L"`r drop_cap(", 11) == 0)
            {
            std::advance(currentStart, 11);
            if (currentStart + 1 < currentEndSentinel &&
                (currentStart[1] == L'\'' || currentStart[1] == L'"'))
                {
                std::advance(currentStart, 1);
                }
            if (*currentStart == L'\'' || *currentStart == L'"')
                {
                const auto quoteChar{ *currentStart };
                std::advance(currentStart, 1);
                const auto* endOfTag = string_util::find_unescaped_char_same_line_n(
                    currentStart, quoteChar, std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                    {
                    log_message(L"Bad 'r drop cap' code block in markdown file.");
                    return false;
                    }
                [[maybe_unused]]
                const auto* retval = m_subParser->operator()(
                    { currentStart, static_cast<size_t>(std::distance(currentStart, endOfTag)) });
                add_character(L'"');
                add_characters(
                    { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                add_character(L'"');
                if (m_subParser->get_filtered_text_length() != 0U)
                    {
                    previousChar =
                        m_subParser
                            ->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                    }
                endOfTag = string_util::find_unescaped_char_same_line_n(
                    currentStart, L'`', std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                    {
                    log_message(L"Bad 'r drop cap' code block in markdown file.");
                    return false;
                    }
                currentStart = ++endOfTag;
                }
            return true;
            }
        if (std::wcsncmp(currentStart, L"`r menu(", 8) == 0)
            {
            std::advance(currentStart, 8);
            if (currentStart + 1 < currentEndSentinel && (currentStart[0] == L'c') &&
                (currentStart[1] == L'('))
                {
                std::advance(currentStart, 2);
                }
            if (*currentStart == L'\'' || *currentStart == L'"')
                {
                const auto* endOfTag = std::wcsstr(currentStart, L")`");
                if (endOfTag == nullptr)
                    {
                    log_message(L"Bad 'r menu' code block in markdown file.");
                    return false;
                    }
                [[maybe_unused]]
                const auto* retval = m_subParser->operator()(
                    { currentStart,
                      static_cast<size_t>(std::distance(currentStart, endOfTag - 1)) });
                add_characters(
                    { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                if (m_subParser->get_filtered_text_length() != 0U)
                    {
                    previousChar =
                        m_subParser
                            ->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                    }
                currentStart = std::next(endOfTag, 2);
                }
            return true;
            }
        // read content as-is otherwise
        if (std::wcsncmp(currentStart, L"`r ", 3) == 0)
            {
            std::advance(currentStart, 3);
            }
        else if (std::wcsncmp(currentStart, L"`python ", 8) == 0)
            {
            std::advance(currentStart, 8);
            }
        else
            {
            std::advance(currentStart, 1);
            }
        // `` section, which can have embedded backticks
        if (*currentStart == '`')
            {
            const auto* endOfTag = std::wcsstr(++currentStart, L"``");
            if (endOfTag == nullptr || endOfTag > currentEndSentinel)
                {
                log_message(L"Bad inline `` code block in markdown file.");
                return false;
                }
            // read in content verbatim
            while (currentStart < endOfTag)
                {
                previousChar = *currentStart;
                add_character(*currentStart);
                std::advance(currentStart, 1);
                }
            std::advance(currentStart, 2);
            }
        // just a single backtick block, should be on one line
        else
            {
            while (currentStart < currentEndSentinel && *currentStart != '`')
                {
                // inline blocks should be on one line, so bail if we hit a new line
                // as this would probably be a missing closing backtick
                if (*currentStart == L'\n' || *currentStart == L'\r')
                    {
                    log_message(L"Unterminated inline ` code block in markdown file.");
                    previousChar = *currentStart;
                    add_character(*currentStart);
                    std::advance(currentStart, 1);
                    break;
                    }
                previousChar = *currentStart;
                add_character(*currentStart);
                std::advance(currentStart, 1);
                }
            if (*currentStart == '`')
                {
                std::advance(currentStart, 1);
                }
            }
        return true;
        }
    return true;
    }

//--------------------------------------------
size_t lily_of_the_valley::markdown_extract_text::parse_styled_text(std::wstring_view input,
                                                                    wchar_t& previousChar,
                                                                    const wchar_t tag)
    {
    if (input.empty())
        {
        return std::wstring_view::npos;
        }

    const wchar_t* currentStart = input.data();
    const wchar_t* const endSentinel = input.data() + input.size();

    // not styling text, just an orphan character that should be processed as-is
    if (*currentStart == tag && std::next(currentStart) < endSentinel)
        {
        if (currentStart[1] != tag && (std::iswalnum(currentStart[1]) == 0) &&
            currentStart[1] != L'`')
            {
            add_character(*currentStart);
            previousChar = *currentStart;
            return 1;
            }
        }

    const wchar_t* const tagStart{ currentStart };
    while (currentStart < endSentinel && *currentStart == tag)
        {
        std::advance(currentStart, 1);
        }

    const std::wstring_view currentTag{ tagStart, static_cast<size_t>(
                                                      std::distance(tagStart, currentStart)) };

    const wchar_t* endOfTag = string_util::find_unescaped_char_same_line_n(
        currentStart, tag, std::distance(currentStart, endSentinel));

    if (endOfTag == nullptr || endOfTag >= endSentinel)
        {
        log_message(L"Missing matching styling tag in markdown file.");
        return std::wstring_view::npos;
        }

    // if a bold tag (**), then move to the matching (terminating) tag
    while (currentTag == L"**" && (std::next(endOfTag) < endSentinel) &&
           *std::next(endOfTag) != L'*')
        {
        std::advance(endOfTag, 1);
        endOfTag = string_util::find_unescaped_char_same_line_n(
            endOfTag, tag, std::distance(endOfTag, endSentinel));
        if (endOfTag == nullptr || endOfTag >= endSentinel)
            {
            log_message(L"Missing matching styling tag in markdown file.");
            return std::wstring_view::npos;
            }
        }

    // or an italic tag(*), then move to the matching (*), skipping over any
    // embedded bold tags (**)
    while (currentTag == L"*" && (std::next(endOfTag) < endSentinel) &&
           *std::next(endOfTag) == L'*')
        {
        std::advance(endOfTag, 2);
        endOfTag = string_util::find_unescaped_char_same_line_n(
            endOfTag, tag, std::distance(endOfTag, endSentinel));
        if (endOfTag == nullptr || endOfTag >= endSentinel)
            {
            log_message(L"Missing matching styling tag in markdown file.");
            return std::wstring_view::npos;
            }
        }

    while ((endOfTag < endSentinel) && *endOfTag == tag)
        {
        std::advance(endOfTag, 1);
        }

    // in case we stepped ahead, step back to the last closing tag
    if (*endOfTag != tag)
        {
        std::advance(endOfTag, -1);
        }

    // ...and then move back to the first tag of the consecutive closing tags
    while (endOfTag > currentStart && *endOfTag == tag)
        {
        std::advance(endOfTag, -1);
        }

    if (*endOfTag != tag)
        {
        std::advance(endOfTag, 1);
        }

    [[maybe_unused]]
    const auto* retval = m_subParser->operator()(
        { currentStart, static_cast<size_t>(std::distance(currentStart, endOfTag)) });

    add_characters({ m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });

    const wchar_t* newStart = endOfTag;
    while ((newStart < endSentinel) && *newStart == tag)
        {
        std::advance(newStart, 1);
        }

    return static_cast<size_t>(std::distance(input.data(), newStart));
    }

//--------------------------------------------
size_t lily_of_the_valley::markdown_extract_text::parse_html_block(std::wstring_view input,
                                                                   std::wstring_view tag,
                                                                   std::wstring_view endTag)
    {
    if (input.size() < tag.size() + 1)
        {
        return std::wstring_view::npos;
        }

    const std::wstring_view searchView = input.substr(tag.length() + 1); // step over '<' + tag

    const wchar_t* endOfTag = string_util::find_matching_close_tag(searchView, tag, endTag);

    if (endOfTag == nullptr)
        {
        log_message(L"Bad HTML section in markdown file.");
        return std::wstring_view::npos;
        }

    // move past closing tag
    endOfTag += endTag.length();

    const wchar_t* inputEnd = input.data() + input.size();
    if (endOfTag > inputEnd)
        {
        log_message(L"Bad HTML section in markdown file.");
        return std::wstring_view::npos;
        }

    const auto consumed = static_cast<size_t>(std::distance(input.data(), endOfTag));

    html_extract_text htmlExtract;
    htmlExtract(input.data(), consumed, false, false);

    add_characters({ htmlExtract.get_filtered_text(), htmlExtract.get_filtered_text_length() });

    return consumed;
    }

//--------------------------------------------
size_t
lily_of_the_valley::markdown_extract_text::find_metadata_section_end(std::wstring_view mdText)
    {
    if (mdText.empty())
        {
        return std::wstring_view::npos;
        }

    // step over first line
    const size_t firstEol = mdText.find_first_of(L"\r\n");
    if (firstEol == std::wstring_view::npos)
        {
        return std::wstring_view::npos;
        }

    // find terminating "---" line after that
    const size_t yamlEndMarker = mdText.find(L"\n---", firstEol);
    if (yamlEndMarker == std::wstring_view::npos)
        {
        return std::wstring_view::npos;
        }

    // move past "\n---"
    size_t pos = yamlEndMarker + 4;

    // find end of that line
    const size_t endOfLine = mdText.find_first_of(L"\r\n", pos);
    if (endOfLine == std::wstring_view::npos)
        {
        return std::wstring_view::npos;
        }

    // skip trailing newlines
    pos = endOfLine;
    while (pos < mdText.size() && string_util::is_either(mdText[pos], L'\r', L'\n'))
        {
        ++pos;
        }

    return pos;
    }

//--------------------------------------------
const wchar_t* lily_of_the_valley::markdown_extract_text::operator()(std::wstring_view md_text)
    {
    clear_log();
    clear();
    const wchar_t* currentStart{ nullptr };
    const wchar_t* currentEndSentinel{ nullptr };

    if (md_text.empty())
        {
        return nullptr;
        }

    if (m_subParser == nullptr)
        {
        m_subParser = std::make_unique<markdown_extract_text>();
        }

    allocate_text_buffer(md_text.length() * 2);

    if (has_metadata_section(md_text))
        {
        const auto metaEnd = find_metadata_section_end(md_text);
        if (metaEnd != std::wstring_view::npos)
            {
            md_text.remove_prefix(metaEnd);
            }
        }

    // in case metadata section ate up the whole file
    // (or at least the part of the file requested to be reviewed)
    if (md_text.empty())
        {
        return get_filtered_text();
        }

    // find the currentStart of the text body and set up where we halt our searching
    currentStart = md_text.data();
    currentEndSentinel = md_text.data() + md_text.length();

    constexpr std::wstring_view BEGIN_FIGURE{ L"\\begin{figure}" };
    constexpr std::wstring_view END_FIGURE{ L"\\end{figure}" };

    constexpr std::wstring_view TABLE{ L"table" };
    constexpr std::wstring_view TABLE_END{ L"</table>" };

    constexpr std::wstring_view UNORDERED_LIST{ L"ul" };
    constexpr std::wstring_view UNORDERED_LIST_END{ L"</ul>" };

    constexpr std::wstring_view ORDERED_LIST{ L"ol" };
    constexpr std::wstring_view ORDERED_LIST_END{ L"</ol>" };

    constexpr std::wstring_view SUP{ L"sup" };
    constexpr std::wstring_view SUP_END{ L"</sup>" };

    bool isEscaping{ false };
    bool headerMode{ false };
    wchar_t previousChar{ L'\n' };

    while (currentStart != nullptr && (currentStart < currentEndSentinel) && (*currentStart != 0))
        {
        if (*currentStart == L'\\')
            {
            // Previous character was not \, but this one is.
            // Skip and get ready to escape the next character.
            if (!isEscaping)
                {
                // remove \index{} tags
                if (std::wcsncmp(currentStart, L"\\index{", 7) == 0)
                    {
                    std::advance(currentStart, 7);
                    const auto* endOfTag = string_util::find_unescaped_matching_close_tag_same_line(
                        currentStart, L'{', L'}');
                    if (endOfTag == nullptr)
                        {
                        log_message(L"Bad index{} command in markdown file.");
                        break;
                        }
                    currentStart = ++endOfTag;
                    continue;
                    }
                if (std::wcsncmp(currentStart, BEGIN_FIGURE.data(), BEGIN_FIGURE.length()) == 0)
                    {
                    std::advance(currentStart, BEGIN_FIGURE.length());
                    continue;
                    }
                if (std::wcsncmp(currentStart, END_FIGURE.data(), END_FIGURE.length()) == 0)
                    {
                    std::advance(currentStart, END_FIGURE.length());
                    continue;
                    }
                if (std::wcsncmp(currentStart, L"\\@ref(", 6) == 0)
                    {
                    std::advance(currentStart, 6);
                    const auto* endOfTag = string_util::find_unescaped_matching_close_tag_same_line(
                        currentStart, L'(', L')');
                    if (endOfTag == nullptr)
                        {
                        log_message(L"Bad cross reference command in markdown file.");
                        break;
                        }
                    currentStart = ++endOfTag;
                    continue;
                    }
                if (std::wcsncmp(currentStart, L"\\newpage", 8) == 0)
                    {
                    std::advance(currentStart, 8);
                    add_characters(L"\n\n");
                    continue;
                    }
                if (currentStart < currentEndSentinel &&
                    (currentStart[1] == L'\n' || currentStart[1] == L'\r'))
                    {
                    headerMode = true;
                    std::advance(currentStart, 1);
                    continue;
                    }
                // actually is an escape character
                isEscaping = true;
                previousChar = *currentStart;
                std::advance(currentStart, 1);
                continue;
                }
            }
        // skip over header tags
        else if (*currentStart == L'#')
            {
            if (!isEscaping && (previousChar == L'\n' || previousChar == L'\r'))
                {
                while ((currentStart < currentEndSentinel) && *currentStart == L'#')
                    {
                    std::advance(currentStart, 1);
                    }
                // space between # and header text
                while ((*currentStart == L' ' || *currentStart == L'\t') &&
                       (currentStart < currentEndSentinel))
                    {
                    std::advance(currentStart, 1);
                    }
                previousChar = *currentStart;
                headerMode = true;
                continue;
                }
            }
        // RMarkdown div fences
        else if (*currentStart == L':')
            {
            if (!isEscaping && (previousChar == L'\n' || previousChar == L'\r'))
                {
                // space between > and quote text
                while ((currentStart < currentEndSentinel) && (*currentStart == L':'))
                    {
                    std::advance(currentStart, 1);
                    }
                continue;
                }
            }
        // block quotes
        else if (*currentStart == L'>')
            {
            if (!isEscaping && (previousChar == L'\n' || previousChar == L'\r'))
                {
                size_t tabCount{ 0 };
                while ((currentStart < currentEndSentinel) && *currentStart == L'>')
                    {
                    ++tabCount;
                    std::advance(currentStart, 1);
                    }
                // space between > and quote text
                while ((*currentStart == L' ' || *currentStart == L'\t') &&
                       (currentStart < currentEndSentinel))
                    {
                    std::advance(currentStart, 1);
                    }
                fill_with_character(tabCount, L'\t');
                // Flags that we are still at the currentStart of the line,
                // so that headers and list items can still be parsed correctly.
                previousChar = L'\n';
                continue;
                }
            }
        // block quotes
        else if (*currentStart == L'&')
            {
            if (!isEscaping)
                {
                const auto* endOfTag = std::wcsstr(currentStart, L";");
                if (endOfTag != nullptr && (endOfTag < currentEndSentinel) &&
                    std::distance(currentStart, endOfTag) <= 6)
                    {
                    if (currentStart + 3 < endOfTag && currentStart[1] == L'#')
                        {
                        const wchar_t decodedChar =
                            string_util::is_either(currentStart[2], L'x', L'X') ?
                                // if it is hex encoded (e.g., '&#xFF')
                                static_cast<wchar_t>(std::wcstol(currentStart + 3, nullptr, 16)) :
                                // else it is a plain numeric value (e.g., '&#79')
                                static_cast<wchar_t>(std::wcstol(currentStart + 2, nullptr, 10));
                        if (decodedChar != 0)
                            {
                            add_character(decodedChar);
                            }
                        previousChar = decodedChar;
                        }
                    else
                        {
                        const auto decodedChar = html_extract_text::HTML_TABLE_LOOKUP.find(
                            { currentStart + 1,
                              static_cast<size_t>(std::distance(currentStart, endOfTag) - 1) });
                        add_character(decodedChar);
                        previousChar = decodedChar;
                        }
                    std::advance(currentStart, std::distance(currentStart, endOfTag) + 1);
                    continue;
                    }
                // not an HTML entity, treat as an ampersand at the end of the loop
                }
            }
        // code blocks
        else if (*currentStart == L'`')
            {
            if (!parse_code_block(isEscaping, currentEndSentinel, currentStart, previousChar,
                                  headerMode))
                {
                break;
                }
            continue;
            }
        // images (we don't read in the alt text inside the [], just skip everything)
        else if (*currentStart == L'!')
            {
            if (!isEscaping && (currentStart + 1 < currentEndSentinel) && currentStart[1] == L'[')
                {
                std::advance(currentStart, 2);
                const auto* endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                    currentStart, L'[', L']', std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                    {
                    log_message(L"Bad image command in markdown file.");
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                currentStart = ++endOfTag;
                if (*currentStart == L'(')
                    {
                    std::advance(currentStart, 1);
                    endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                        currentStart, L'(', L')', std::distance(currentStart, currentEndSentinel));
                    if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                        {
                        log_message(L"Bad image command in markdown file.");
                        previousChar = L'(';
                        add_character(L'(');
                        continue;
                        }
                    currentStart = ++endOfTag;
                    }
                continue;
                }
            }
        // links
        else if (*currentStart == L'[')
            {
            if (!isEscaping)
                {
                const auto* labelStart{ ++currentStart };
                const auto* endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                    currentStart, L'[', L']', std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                    {
                    log_message(L"Bad link command in markdown file. Missing closing ']'.");
                    // just treat it like a stray '[' and keep going
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                currentStart = ++endOfTag;
                if (*currentStart == L'(')
                    {
                    const auto* labelEnd{ currentStart - 1 };
                    std::advance(currentStart, 1);
                    endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                        currentStart, L'(', L')', std::distance(currentStart, currentEndSentinel));
                    if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                        {
                        log_message(L"Bad link command in markdown file. Missing closing ')'.");
                        // read in the label and '(' section after it as-is if closing ')'
                        // is missing
                        currentStart = labelStart;
                        previousChar = L'[';
                        add_character(L'[');
                        continue;
                        }
                    currentStart = ++endOfTag;
                    if (labelStart < labelEnd)
                        {
                        [[maybe_unused]]
                        const auto* retval = m_subParser->operator()(
                            { labelStart,
                              static_cast<size_t>(std::distance(labelStart, labelEnd)) });
                        add_characters({ m_subParser->get_filtered_text(),
                                         m_subParser->get_filtered_text_length() });
                        if (m_subParser->get_filtered_text_length() != 0U)
                            {
                            previousChar =
                                m_subParser
                                    ->get_filtered_text()[m_subParser->get_filtered_text_length() -
                                                          1];
                            }
                        }
                    }
                else
                    {
                    log_message(L"Bad link command in markdown file. Missing '()' section.");
                    // read in the label and '(' section after it as-is if closing ')' is missing
                    currentStart = labelStart;
                    previousChar = L'[';
                    add_character(L'[');
                    }
                continue;
                }
            }
        // IDs
        else if (*currentStart == L'{')
            {
            if (!isEscaping)
                {
                // Quarto shortcodes: {{< name args >}}
                if (std::wcsncmp(currentStart, L"{{< ", 4) == 0 ||
                    std::wcsncmp(currentStart, L"{{<\t", 4) == 0)
                    {
                    const auto* endOfShortcode = std::wcsstr(currentStart, L">}}");
                    if (endOfShortcode == nullptr || endOfShortcode >= currentEndSentinel)
                        {
                        log_message(L"Bad Quarto shortcode in markdown file.");
                        break;
                        }
                    const auto* scStart = std::next(currentStart, 4); // past "{{< "
                    const auto* scEnd = endOfShortcode;               // at ">}}"
                    // trim trailing space before >}}
                    while (scEnd > scStart &&
                           (*std::prev(scEnd, 1) == L' ' || *std::prev(scEnd, 1) == L'\t'))
                        {
                        --scEnd;
                        }
                    const std::wstring_view scContent{ scStart, static_cast<size_t>(std::distance(
                                                                    scStart, scEnd)) };
                    // find shortcode name (first token)
                    size_t nameEnd = scContent.find_first_of(L" \t");
                    const std::wstring_view scName = (nameEnd == std::wstring_view::npos) ?
                                                         scContent :
                                                         scContent.substr(0, nameEnd);
                    if (scName == L"pagebreak")
                        {
                        add_characters(L"\n\n");
                        }
                    else if (scName == L"kbd" || scName == L"meta" || scName == L"var" ||
                             scName == L"env")
                        {
                        // extract arguments after the name, uppercased
                        if (nameEnd != std::wstring_view::npos && nameEnd + 1 < scContent.size())
                            {
                            auto args = scContent.substr(nameEnd + 1);
                            for (const auto ch : args)
                                {
                                add_character(static_cast<wchar_t>(std::towupper(ch)));
                                }
                            if (!args.empty())
                                {
                                previousChar = args.back();
                                }
                            }
                        }
                    else if (scName == L"video")
                        {
                        // extract URL as-is
                        if (nameEnd != std::wstring_view::npos && nameEnd + 1 < scContent.size())
                            {
                            auto args = scContent.substr(nameEnd + 1);
                            add_characters(args);
                            if (!args.empty())
                                {
                                previousChar = args.back();
                                }
                            }
                        }
                    // else: unknown shortcode, strip entirely
                    currentStart = std::next(endOfShortcode, 3); // past ">}}"
                    }
                else
                    {
                    std::advance(currentStart, 1);
                    const auto* endOfTag =
                        string_util::find_unescaped_matching_close_tag_same_line_n(
                            currentStart, L'{', L'}',
                            std::distance(currentStart, currentEndSentinel));
                    if (endOfTag == nullptr || (endOfTag >= currentEndSentinel))
                        {
                        log_message(L"Bad ID command in markdown file.");
                        break;
                        }
                    currentStart = ++endOfTag;
                    }
                continue;
                }
            }
        // superscript (just read as-is)
        else if (!isEscaping && *currentStart == L'^')
            {
            std::advance(currentStart, 1);
            continue;
            }
        // Quarto cross references (figures, tables, sections, etc.)
        else if (!isEscaping && *currentStart == L'@')
            {
            const auto* refStart = std::next(currentStart);
            // scan past lowercase ASCII letters to find the would-be prefix
            const auto* prefixEnd = refStart;
            while (prefixEnd < currentEndSentinel && *prefixEnd >= L'a' && *prefixEnd <= L'z')
                {
                ++prefixEnd;
                }
            // if followed by '-', check whether the prefix is a known cross-ref type
            if (prefixEnd < currentEndSentinel && *prefixEnd == L'-' && prefixEnd > refStart)
                {
                static const std::set<std::wstring_view> QUARTO_XREF_PREFIXES{
                    L"fig", L"tbl", L"sec", L"eq",  L"lst", L"thm", L"lem",
                    L"cor", L"prp", L"cnj", L"def", L"exm", L"exr"
                };
                const std::wstring_view candidate{ refStart, static_cast<size_t>(std::distance(
                                                                 refStart, prefixEnd)) };
                if (QUARTO_XREF_PREFIXES.contains(candidate))
                    {
                    // -@ref suppresses the type label in Quarto;
                    // remove the preceding '-' if it was already written
                    if (get_filtered_text_length() > 0 && get_filtered_buffer().back() == L'-')
                        {
                        get_filtered_buffer().pop_back();
                        }
                    // skip the entire @prefix-identifier
                    const auto* refEnd = prefixEnd;
                    while (refEnd < currentEndSentinel &&
                           (std::iswalnum(*refEnd) != 0 || *refEnd == L'-' || *refEnd == L'_'))
                        {
                        ++refEnd;
                        }
                    currentStart = refEnd;
                    continue;
                    }
                }
            }
        // display math ($$...$$)
        else if (!isEscaping && std::wcsncmp(currentStart, L"$$", 2) == 0)
            {
            std::advance(currentStart, 2);
            // skip optional newline after opening $$
            while (currentStart < currentEndSentinel &&
                   (*currentStart == L'\r' || *currentStart == L'\n'))
                {
                std::advance(currentStart, 1);
                }
            const auto* endOfTag = std::wcsstr(currentStart, L"$$");
            if (endOfTag == nullptr || endOfTag >= currentEndSentinel)
                {
                log_message(L"Bad display math block in markdown file.");
                // no matching $$, treat the $$ as literal and continue
                add_characters(L"$$");
                previousChar = L'$';
                continue;
                }
            // trim trailing newlines before closing $$
            const auto* contentEnd{ endOfTag };
            while (contentEnd > currentStart &&
                   (*(contentEnd - 1) == L'\r' || *(contentEnd - 1) == L'\n'))
                {
                --contentEnd;
                }
            std::wstring equation(currentStart, contentEnd);
            if (!equation.empty())
                {
                transform_equation(equation);
                add_characters({ equation.data(), equation.length() });
                previousChar = equation.back();
                }
            currentStart = std::next(endOfTag, 2);
            continue;
            }
        // inline math ($...$)
        else if (!isEscaping && *currentStart == L'$')
            {
            // opening $ must not be followed by whitespace
            if (std::next(currentStart) < currentEndSentinel && *std::next(currentStart) != L' ' &&
                *std::next(currentStart) != L'\t' && *std::next(currentStart) != L'\r' &&
                *std::next(currentStart) != L'\n')
                {
                const auto* scanStart = std::next(currentStart);
                const auto* endOfTag = string_util::find_unescaped_char_same_line_n(
                    scanStart, L'$', std::distance(scanStart, currentEndSentinel));
                // closing $ must not be preceded by whitespace and not followed by a digit
                if (endOfTag != nullptr && endOfTag < currentEndSentinel && endOfTag > scanStart &&
                    *std::prev(endOfTag, 1) != L' ' && *std::prev(endOfTag, 1) != L'\t' &&
                    (std::next(endOfTag) >= currentEndSentinel ||
                     std::iswdigit(*std::next(endOfTag)) == 0))
                    {
                    std::wstring equation(scanStart, endOfTag);
                    if (!equation.empty())
                        {
                        transform_equation(equation);
                        add_characters({ equation.data(), equation.length() });
                        previousChar = equation.back();
                        }
                    currentStart = std::next(endOfTag);
                    continue;
                    }
                }
            // not math, treat $ as literal character
            }
        // RMarkdown (Pandoc) comment
        else if (!isEscaping && std::wcsncmp(currentStart, L"<!--", 4) == 0)
            {
            const auto* const endOfTag = std::wcsstr(currentStart, L"-->");
            if (endOfTag == nullptr)
                {
                log_message(L"Bad comment block in markdown file.");
                break;
                }
            currentStart = std::next(endOfTag, 3);
            continue;
            }
        else if (!isEscaping && std::wcsncmp(currentStart, L"<p>", 3) == 0)
            {
            std::advance(currentStart, 3);
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping && std::wcsncmp(currentStart, L"</p>", 4) == 0)
            {
            std::advance(currentStart, 4);
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        // newline hacks found in tables (just replace with space to keep the table structure).
        else if (!isEscaping && std::wcsncmp(currentStart, L"<br>\\linebreak", 14) == 0)
            {
            std::advance(currentStart, 14);
            previousChar = L' ';
            add_character(L' ');
            continue;
            }
        // HTML newline
        else if (!isEscaping && std::wcsncmp(currentStart, L"<br>", 4) == 0)
            {
            std::advance(currentStart, 4);
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping && std::wcsncmp(currentStart, L"<br/>", 5) == 0)
            {
            std::advance(currentStart, 5);
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping && (std::wcsncmp(currentStart, L"< br/>", 6) == 0 ||
                                 std::wcsncmp(currentStart, L"<br />", 6) == 0))
            {
            std::advance(currentStart, 6);
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (*currentStart == L'<')
            {
            if (!isEscaping &&
                static_cast<size_t>(std::distance(currentStart, currentEndSentinel)) >=
                    TABLE.length() + 1 &&
                std::wcsncmp(std::next(currentStart), TABLE.data(), TABLE.length()) == 0)
                {
                const size_t consumed = parse_html_block(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    TABLE, TABLE_END);
                if (consumed == std::wstring_view::npos)
                    {
                    break;
                    }
                std::advance(currentStart, consumed);
                }
            else if (!isEscaping &&
                     static_cast<size_t>(std::distance(currentStart, currentEndSentinel)) >=
                         UNORDERED_LIST.length() + 1 &&
                     std::wcsncmp(std::next(currentStart), UNORDERED_LIST.data(),
                                  UNORDERED_LIST.length()) == 0)
                {
                const size_t consumed = parse_html_block(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    UNORDERED_LIST, UNORDERED_LIST_END);
                if (consumed == std::wstring_view::npos)
                    {
                    break;
                    }
                std::advance(currentStart, consumed);
                }
            else if (!isEscaping &&
                     static_cast<size_t>(std::distance(currentStart, currentEndSentinel)) >=
                         ORDERED_LIST.length() + 1 &&
                     std::wcsncmp(std::next(currentStart), ORDERED_LIST.data(),
                                  ORDERED_LIST.length()) == 0)
                {
                const size_t consumed = parse_html_block(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    ORDERED_LIST, ORDERED_LIST_END);
                if (consumed == std::wstring_view::npos)
                    {
                    break;
                    }
                std::advance(currentStart, consumed);
                }
            else if (!isEscaping &&
                     static_cast<size_t>(std::distance(currentStart, currentEndSentinel)) >=
                         SUP.length() + 1 &&
                     std::wcsncmp(std::next(currentStart), SUP.data(), SUP.length()) == 0)
                {
                const size_t consumed = parse_html_block(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    SUP, SUP_END);
                if (consumed == std::wstring_view::npos)
                    {
                    break;
                    }
                std::advance(currentStart, consumed);
                }
            else if (!isEscaping && std::next(currentStart) < currentEndSentinel &&
                     (*std::next(currentStart) == L'/' || *std::next(currentStart) == L'p' ||
                      std::wcsncmp(std::next(currentStart), L"a ", 2) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"b>", 2) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"i>", 2) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"u>", 2) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"code", 4) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"span", 4) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"strong", 6) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"div", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"dl>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"dt>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"dd>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"em>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"tt>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"ul>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"ol>", 3) == 0 ||
                      std::wcsncmp(std::next(currentStart), L"li>", 3) == 0))
                {
                std::advance(currentStart, 1);
                const auto* endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                    currentStart, L'<', L'>', std::distance(currentStart, currentEndSentinel));
                if (endOfTag == nullptr)
                    {
                    log_message(L"Bad <> pair in markdown file.");
                    break;
                    }
                currentStart = ++endOfTag;
                continue;
                }
            }
        // newlines
        else if (*currentStart == L'\n' || *currentStart == L'\r')
            {
            // two (or more) spaces at the end of a line indicates a paragraph break
            size_t newlineCount{ 0 };
            if (previousChar == L' ' && std::distance(md_text.data(), currentStart) > 2 &&
                *(currentStart - 2) == L' ')
                {
                ++newlineCount;
                }
            // count the newlines (taking CRLF combos into account)
            while ((*currentStart == L'\n' || *currentStart == L'\r') &&
                   (currentStart < currentEndSentinel))
                {
                if (*currentStart == L'\r' && (currentStart + 1 < currentEndSentinel) &&
                    currentStart[1] == L'\n')
                    {
                    std::advance(currentStart, 1);
                    continue;
                    }
                ++newlineCount;
                std::advance(currentStart, 1);
                // If the next line is a header line divider, then
                // skip that, switch to header mode, and keep reading
                // any more newlines
                if (currentStart + 1 < currentEndSentinel &&
                    ((currentStart[0] == L'=' && currentStart[1] == L'=') ||
                     (currentStart[0] == L'-' && currentStart[1] == L'-')))
                    {
                    while ((*currentStart == L'=' || *currentStart == L'-') &&
                           (currentStart < currentEndSentinel))
                        {
                        std::advance(currentStart, 1);
                        }
                    headerMode = true;
                    }
                }

            const auto* scanAhead{ currentStart };
            size_t leadingSpaces{ 0 };
            while ((scanAhead < currentEndSentinel) && (*scanAhead == L' ' || *scanAhead == L'\t'))
                {
                ++scanAhead;
                ++leadingSpaces;
                }
            if (newlineCount == 1 && headerMode)
                {
                add_characters(L"\n\n");
                previousChar = L'\n';
                }
            // next line starts a list item, quote block, table, etc., so keep the newline as-is
            else if (newlineCount == 1 &&
                     (string_util::is_one_of(*scanAhead, L">-*+|:^") || leadingSpaces >= 4))
                {
                fill_with_character(newlineCount, L'\n');
                previousChar = L'\n';
                }
            // same for an ordered list
            else if (newlineCount == 1 && (currentStart < currentEndSentinel) &&
                     (std::iswdigit(*currentStart) != 0))
                {
                const auto* scanAheadDigit{ currentStart };
                while ((scanAheadDigit < currentEndSentinel) &&
                       (std::iswdigit(*scanAheadDigit) != 0))
                    {
                    ++scanAheadDigit;
                    }
                if (*scanAheadDigit == L'.')
                    {
                    fill_with_character(newlineCount, L'\n');
                    previousChar = L'\n';
                    }
                // not an ordered list, default behavior to read as space
                else
                    {
                    add_character(L' ');
                    previousChar = L' ';
                    }
                }
            // a single newline not at end of a self-contained line
            // (e.g., a header) is seen as a space
            else if (newlineCount == 1)
                {
                if (*currentStart == L'#' || *currentStart == L'-' || *currentStart == L'|')
                    {
                    add_character(L'\n');
                    previousChar = L'\n';
                    }
                else
                    {
                    add_character(L' ');
                    previousChar = L' ';
                    }
                }
            else
                {
                fill_with_character(newlineCount, L'\n');
                previousChar = L'\n';
                }
            headerMode = false;
            isEscaping = false;
            continue;
            }
        // styling tags that just get removed from raw text
        else if (!isEscaping && *currentStart == L'*')
            {
            if (const size_t consumed = parse_styled_text(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    previousChar, L'*');
                consumed != std::wstring_view::npos)
                {
                std::advance(currentStart, consumed);
                continue;
                }
            // malformed styling: treat '*' as literal
            // malformed styling: skip stray tag characters
            while (currentStart < currentEndSentinel && *currentStart == L'*')
                {
                std::advance(currentStart, 1);
                }
            continue;
            }
        else if (!isEscaping && *currentStart == L'_')
            {
            if (const size_t consumed = parse_styled_text(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    previousChar, L'_');
                consumed != std::wstring_view::npos)
                {
                std::advance(currentStart, consumed);
                continue;
                }
            // malformed styling: skip stray tag characters
            while (currentStart < currentEndSentinel && *currentStart == L'_')
                {
                std::advance(currentStart, 1);
                }
            continue;
            }
        else if (!isEscaping && *currentStart == L'~')
            {
            if (const size_t consumed = parse_styled_text(
                    std::wstring_view{ currentStart,
                                       static_cast<size_t>(currentEndSentinel - currentStart) },
                    previousChar, L'~');
                consumed != std::wstring_view::npos)
                {
                std::advance(currentStart, consumed);
                continue;
                }
            // malformed styling: skip stray tag characters
            while (currentStart < currentEndSentinel && *currentStart == L'~')
                {
                std::advance(currentStart, 1);
                }
            continue;
            }
        // table
        else if (!isEscaping && (*currentStart == L'|'))
            {
            previousChar = L'|';
            const auto* scanAhead{ std::next(currentStart) };
            // if the line is table column format specifiers (e.g., ":--"),
            // then step over the whole line
            while (scanAhead < currentEndSentinel && (*scanAhead == L' ' || *scanAhead == L'\t'))
                {
                ++scanAhead;
                }
            if ((*scanAhead == L'-' && *std::next(scanAhead) == L'-') ||
                (*scanAhead == L':' && *std::next(scanAhead) == L'-'))
                {
                // move to the end of the line...
                while (scanAhead < currentEndSentinel &&
                       !(*scanAhead == L'\r' || *scanAhead == L'\n'))
                    {
                    ++scanAhead;
                    }
                // ...and then step of the newlines
                while (scanAhead < currentEndSentinel &&
                       (*scanAhead == L'\r' || *scanAhead == L'\n'))
                    {
                    ++scanAhead;
                    }
                currentStart = scanAhead;
                }
            else
                {
                add_characters(L"\t|");
                std::advance(currentStart, 1);
                }
            continue;
            }
        // turn off escaping and load the character
        isEscaping = false;
        if (currentStart < currentEndSentinel)
            {
            previousChar = *currentStart;
            add_character(*currentStart);
            std::advance(currentStart, 1);
            }
        else
            {
            break;
            }
        }

    return get_filtered_text();
    }
