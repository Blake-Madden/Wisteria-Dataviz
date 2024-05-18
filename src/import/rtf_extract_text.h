/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __RTF_EXTRACT_H__
#define __RTF_EXTRACT_H__

#include "../i18n-check/src/string_util.h"
#include "extract_text.h"
#include "html_extract_text.h"
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace lily_of_the_valley
    {
    // CHaracter Properties.
    /// @private
    struct char_prop
        {
        char fBold{ 0 };
        char fUnderline{ 0 };
        char fItalic{ 0 };
        char fStrikeThrough{ 0 };
        };

    /// @private
    enum class JUST
        {
        justL,
        justR,
        justC,
        justF
        };

    // PAragraph Properties.
    /// @private
    struct para_prop
        {
        int xaLeft{ 0 };          // left indent in twips
        int xaRight{ 0 };         // right indent in twips
        int xaFirst{ 0 };         // first line indent in twips
        JUST just{ JUST::justL }; // justification
        };

    /// @private
    enum class SBK
        {
        sbkNon,
        sbkCol,
        sbkEvn,
        sbkOdd,
        sbkPg
        };
    /// @private
    enum class PGN
        {
        pgDec,
        pgURom,
        pgLRom,
        pgULtr,
        pgLLtr
        };

    // SEction Properties.
    /// @private
    struct SEP
        {
        int cCols{ 0 };              // number of columns
        SBK sbk{ SBK::sbkNon };      // section break type
        int xaPgn{ 0 };              // x position of page number in twips
        int yaPgn{ 0 };              // y position of page number in twips
        PGN pgnFormat{ PGN::pgDec }; // how the page number is formatted
        };

    // DOcument Properties.
    /// @private
    struct DOP
        {
        int xaPage{ 0 };      // page width in twips
        int yaPage{ 0 };      // page height in twips
        int xaLeft{ 0 };      // left margin in twips
        int yaTop{ 0 };       // top margin in twips
        int xaRight{ 0 };     // right margin in twips
        int yaBottom{ 0 };    // bottom margin in twips
        int pgnStart{ 0 };    // starting page number in twips
        char fFacingp{ 0 };   // facing pages enabled?
        char fLandscape{ 0 }; // landscape or portrait?
        };

    // Rtf Destination State
    /// @private
    enum class RDS
        {
        rdsNorm,
        rdsSkip
        };

    // Rtf Internal State
    /// @private
    enum class RIS
        {
        risNorm,
        risBin,
        risHex
        };

    // Property save structure.
    /// @private
    struct SAVE
        {
        struct SAVE* pNext{ nullptr }; // next save
        char_prop chp;
        para_prop pap;
        SEP sep;
        DOP dop;
        RDS rds{ RDS::rdsNorm };
        RIS ris{ RIS::risNorm };
        };

    // What types of properties are there?
    /// @private
    enum class IPROP
        {
        ipropBold,
        ipropItalic,
        ipropUnderline,
        ipropStrikeThrough,
        ipropLeftInd,
        ipropRightInd,
        ipropFirstInd,
        ipropCols,
        ipropPgnX,
        ipropPgnY,
        ipropXaPage,
        ipropYaPage,
        ipropXaLeft,
        ipropXaRight,
        ipropYaTop,
        ipropYaBottom,
        ipropPgnStart,
        ipropSbk,
        ipropPgnFormat,
        ipropFacingp,
        ipropLandscape,
        ipropJust,
        ipropPard,
        ipropPlain,
        ipropSectd,
        ipropMax
        };
    /// @private
    enum class ACTN
        {
        actnSpec,
        actnByte,
        actnWord
        };
    /// @private
    enum class PROPTYPE
        {
        propChp,
        propPap,
        propSep,
        propDop
        };

    /// @private
    struct propmod
        {
        ACTN actn{ ACTN::actnByte };        // size of value
        PROPTYPE prop{ PROPTYPE::propChp }; // structure containing value
        int offset{ 0 };                    // offset of value from base of structure
        };

    /// @private
    enum class IPFN
        {
        ipfnBin,
        ipfnHex,
        ipfnSkipDest
        };
    /// @private
    enum class IDEST
        {
        idestPict,
        idestSkip
        };
    /// @private
    enum class KWD
        {
        kwdChar,
        kwdDest,
        kwdProp,
        kwdSpec,
        kwdString,
        kwdHighlight,
        kwdBold,
        kwdFontColor,
        kwdStrikeThrough,
        kwdItalic,
        kwdUnderline,
        kwdSectionSkip
        };

    /// @private
    inline void reset_property(para_prop& prop)
        {
        prop.xaLeft = prop.xaRight = prop.xaFirst = 0;
        prop.just = JUST::justL;
        }

    /// @private
    inline void reset_property(char_prop& prop)
        {
        prop.fBold = prop.fItalic = prop.fStrikeThrough = prop.fUnderline = 0;
        }

    /// @private
    inline void reset_property(SEP& prop)
        {
        prop.cCols = prop.xaPgn = prop.yaPgn = 0;
        prop.sbk = SBK::sbkNon;
        prop.pgnFormat = PGN::pgDec;
        }

    /// @private
    inline void reset_property(DOP& prop)
        {
        prop.fFacingp = prop.fLandscape = 0;
        prop.pgnStart = prop.xaLeft = prop.xaPage = prop.xaRight = prop.yaBottom = prop.yaPage =
            prop.yaTop = 0;
        }

    /// @private
    inline void reset_property(SAVE& prop)
        {
        prop.pNext = nullptr;
        reset_property(prop.chp);
        reset_property(prop.pap);
        reset_property(prop.sep);
        reset_property(prop.dop);
        prop.rds = RDS::rdsNorm;
        prop.ris = RIS::risNorm;
        }

    // RTF command/value structure.
    /// @private
    struct rtf_symbol
        {
        rtf_symbol(const char* keyword, const int dfault, const bool passDflt, const KWD command,
                   const int index, const wchar_t* stringToPrint)
            : szKeyword(keyword), dflt(dfault), fPassDflt(passDflt), kwd(command), idx(index),
              printString(stringToPrint)
            {
            }

        rtf_symbol() = delete;

        [[nodiscard]]
        bool
        operator<(const rtf_symbol& that) const noexcept
            {
            return (szKeyword.compare(that.szKeyword) < 0);
            }

        std::string szKeyword;    // RTF keyword
        int dflt{ 0 };            // default value to use
        bool fPassDflt{ true };   // true to use default value from this table
        KWD kwd{ KWD::kwdChar };  // base action to take
        int idx{ 0 };             // index into property table if kwd == kwdProp
                                  // index into destination table if kwd == kwdDest
                                  // character to print if kwd == kwdChar
        std::wstring printString; // string to print if kwd == kwdString
        };

    // Color structure used for fonts.
    /// @private
    struct rtf_color
        {
        rtf_color() = default;
        int red{ 0 };
        int green{ 0 };
        int blue{ 0 };
        std::wstring web_color;
        };

    // Keyword descriptions.
    /// @private
    class rtf_symbol_table
        {
      public:
        [[nodiscard]]
        bool is_not_found(const std::set<rtf_symbol>::const_iterator& pos) const noexcept
            {
            return (pos == m_symbols.cend());
            }

        [[nodiscard]]
        std::set<rtf_symbol>::const_iterator find(const char* keyword) const
            {
            return m_symbols.find(rtf_symbol(keyword,
                                             /*the rest of this is filler*/ 0, false, KWD::kwdChar,
                                             0, L""));
            }

      protected:
        std::set<rtf_symbol> m_symbols;
        };

    // RTF to text filter.
    /// @private
    class rtf_to_text_symbol_table final : public rtf_symbol_table
        {
      public:
        rtf_to_text_symbol_table();
        };

    // RTF to HTML filter.
    /// @private
    class rtf_to_html_symbol_table final : public rtf_symbol_table
        {
      public:
        rtf_to_html_symbol_table();
        };

    /** @brief Class to extract text from an RTF stream.
        @details Also offers rudimentary support for converting RTF to HTML.
        @note The HTML converter supports font properties like color,
            background color bold, italics, and underlining,
            encoding them in span tags. RTF tags need to be enclosed in `{}` environments
            for the HTML conversion to work properly, rather than terminating tags (e.g., `\cb0`).
            This ensures that terminating `</span>` tags appear in the proper order.

        @par References
        Adapted from the MSDN example code from
       http://msdn.microsoft.com/en-us/library/aa140300#rtfspec_60.

        RTF reference:
        https://www.oreilly.com/library/view/rtf-pocket-guide/9781449302047/ch01.html
        */
    class rtf_extract_text final : public extract_text
        {
      public:
        /** @brief Output conversion format for RTF parser.*/
        enum class rtf_extraction_type
            {
            rtf_to_text, /**< Converts RTF to plain text.*/
            rtf_to_html  /**< Converts RTF to HTML.*/
            };

        /// @brief General RTF parsing error.
        class rtfparse_exception : public std::exception
            {
            };

        /// @brief Malformed {} combinations (missing a '}').
        class rtfparse_stack_underflow : public rtfparse_exception
            {
            };

        /// @brief Malformed {} combinations (missing a '{').
        class rtfparse_stack_overflow : public rtfparse_exception
            {
            };

        /// @brief Malformed {} combinations.
        class rtfparse_unmatched_brace : public rtfparse_exception
            {
            };

        /// @brief General RTF parsing error.
        class rtfparse_assertion : public rtfparse_exception
            {
            };

        /// @brief Malformed Hexadecimal formatted value.
        class rtfparse_invalid_hex : public rtfparse_exception
            {
            };

        /// @brief Unknown RTF command that couldn't be resolved.
        class rtfparse_bad_table : public rtfparse_exception
            {
            };

        /** @brief Constructs an RTF parser object.
            @param extraction_type Specifies whether to convert the RTF to plain text or HTML.*/
        explicit rtf_extract_text(
            const rtf_extraction_type& extraction_type = rtf_extraction_type::rtf_to_text) noexcept;

        /** @returns The default font size from the RTF's font information.*/
        [[nodiscard]]
        int get_font_size() const noexcept
            {
            return m_font_size;
            }

        /** @returns The HTML CSS section that was formatted from the RTF's color table.\n
                This is only useful when converting to HTML.*/
        [[nodiscard]]
        const std::wstring& get_style_section() const noexcept
            {
            return m_style_section;
            }

        /** @returns The default font from the RTF's font table, or **Arial** if no font table.*/
        [[nodiscard]]
        std::string get_font() const
            {
            if (m_font_table.size())
                {
                return m_font_table[0];
                }
            else
                {
                return "Arial";
                }
            }

        /** @returns The default font color from the RTF's font information.*/
        [[nodiscard]]
        const rtf_color& get_font_color() const noexcept
            {
            return m_text_color;
            }

        /** @brief Sets the prefix in front of CSS styles.
            @details This is useful if you need to have unique
            CSS tags in your converted text.
            @param prefix The prefix to put in front of CSS style tags.*/
        void set_style_prefix(const std::wstring& prefix) { m_style_prefix = prefix; }

        /** @brief Main interface for extracting plain text from an RTF buffer.
            @param text The RTF text to strip.
            @param text_length The length of the text.
            @returns A pointer to the parsed text, or null upon failure.*/
        const wchar_t* operator()(const char* text, const size_t text_length);

        /** @returns The title from the metadata file or stream.
            @note Must be called after calling operator().*/
        [[nodiscard]]
        const std::wstring& get_title() const noexcept
            {
            return m_title;
            }

        /** @returns The subject from the metadata file or stream.
            @note Must be called after calling operator().*/
        [[nodiscard]]
        const std::wstring& get_subject() const noexcept
            {
            return m_subject;
            }

        /** @returns The author from the metadata file or stream.
            @note Must be called after calling operator().*/
        [[nodiscard]]
        const std::wstring& get_author() const noexcept
            {
            return m_author;
            }

        /** @returns
        The keywords from the metadata file or stream.
            @note Must be called after calling operator().*/
        [[nodiscard]]
        const std::wstring& get_keywords() const noexcept
            {
            return m_keywords;
            }

        /** @returns The comments from the metadata file or stream.
            @note Must be called after calling operator().*/
        [[nodiscard]]
        const std::wstring& get_comments() const noexcept
            {
            return m_comments;
            }

      private:
        // Save relevant info on a linked list of SAVE structures.
        /*****************************************************************************************/
        void ecPushRtfState();

        // Always restore relevant info from the top of the SAVE list.
        /*****************************************************************************************/
        void ecPopRtfState();

        // get a control word (and its associated value) and
        // call ecTranslateKeyword to dispatch the control.
        /*****************************************************************************************/
        void ecParseRtfKeyword();

        // Route the character to the appropriate destination stream.
        /*****************************************************************************************/
        void ecParseChar(int ch);

        // Route the string to the appropriate destination stream.
        /*****************************************************************************************/
        void ecParseString(const wchar_t* text, const size_t length) noexcept;

        /// Processes a font property.
        /*****************************************************************************************/
        void ecProcessFontProperty(const wchar_t* htmlCmd, const size_t htmlCmdLength) noexcept;

        /*****************************************************************************************/
        void ecProcessFontColor(const int idx);

        /*****************************************************************************************/
        void ecProcessHighlight(const int idx);

        // Send a character to the output.
        inline void ecPrintChar(const wchar_t ch) noexcept
            {
            if (ch == 3913 || ch == 3929 || ch == 3928) // avoid embedded header/footer symbols
                {
                return;
                }
            add_character(ch);
            }

        // Send a string to the output.
        inline void ecPrintString(const wchar_t* text, const size_t length) noexcept
            {
            add_characters({ text, length });
            }

        // Set the property identified by _iprop_ to the value _val_.
        /*****************************************************************************************/
        void ecApplyPropChange(IPROP iprop, int val);

        // Set a property that requires code to evaluate.
        /*****************************************************************************************/
        inline void ecParseSpecialProperty(IPROP iprop)
            {
            switch (iprop)
                {
            case IPROP::ipropPard:
                reset_property(m_pap);
                break;
            case IPROP::ipropPlain:
                reset_property(m_chp);
                break;
            case IPROP::ipropSectd:
                reset_property(m_sep);
                break;
            default:
                throw rtfparse_bad_table();
                }
            }

        // Search rgsymRtf for szKeyword and evaluate it appropriately.
        // Inputs:
        // szKeyword:   The RTF control to evaluate.
        // param:       The parameter of the RTF control.
        // fParam:      true if the control had a parameter. (that is, if param is valid)
        //              false if it did not.
        /*****************************************************************************************/
        void ecTranslateKeyword(const char* szKeyword, int param, const bool fParam);

        /*****************************************************************************************/
        inline void ecChangeDest() noexcept
            {
            if (m_rds == RDS::rdsSkip) // if we're skipping text,
                {
                return; // don't do anything
                }

            m_rds = RDS::rdsSkip;
            }

        // Evaluate an RTF control that needs special processing.
        /*****************************************************************************************/
        void ecParseSpecialKeyword(IPFN ipfn);

        /// Resets the metadata (e.g., title, subject, etc.)
        void reset_meta_data() noexcept
            {
            m_subject.clear();
            m_title.clear();
            m_comments.clear();
            m_author.clear();
            m_keywords.clear();
            }

        /** @returns @C true if a character is a letter
                (English alphabet only, and no full-width characters).
            @param ch The letter to be reviewed.*/
        [[nodiscard]]
        static constexpr bool is_alpha_7bit(const wchar_t ch) noexcept
            {
            return (((ch >= 0x41 /*'A'*/) && (ch <= 0x5A /*'Z'*/)) ||
                    ((ch >= 0x61 /*'a'*/) && (ch <= 0x7A /*'z'*/)));
            }

        /** @returns Whether a character is a number (narrow [0-9] characters only).
            @param ch The letter to be reviewed.*/
        [[nodiscard]]
        static constexpr bool is_numeric_7bit(const wchar_t ch) noexcept
            {
            return (ch >= L'0' && ch <= L'9') ? true : false;
            }

        rtf_extraction_type m_extraction_type{ rtf_extraction_type::rtf_to_text };
        RIS m_ris;
        RDS m_rds;
        int m_cGroup{ 0 };
        SAVE* m_psave{ nullptr };
        bool m_fSkipDestIfUnk{ true };
        long m_cbBin{ 0 };
        long m_lParam{ 0 };
        const char* m_rtf_text{ nullptr };
        size_t m_paragraphCount{ 0 };
        int m_font_size{ 8 };
        const rtf_symbol_table* m_keyword_command_table{ nullptr };
        SEP m_sep;
        para_prop m_pap;
        DOP m_dop;
        char_prop m_chp;
        std::wstring m_style_section;
        std::wstring m_style_prefix;
        std::vector<rtf_color> m_color_table;
        std::vector<std::string> m_font_table; // the font face names
        rtf_color m_text_color;
        std::wstring m_title;
        std::wstring m_subject;
        std::wstring m_author;
        std::wstring m_keywords;
        std::wstring m_comments;
        bool m_in_bullet_state{ false };

        /// @brief Keeps track of the number of `<span>` generating
        ///     commands inside of an {} environment.
        class command_stacks
            {
          public:
            /// @brief Marks the start of a new `{}` environment.
            void open_stack() { m_stacks.push_back(0); }

            /// Closes the current `{}` environment.
            /// @returns The closing `</span>`s for the current environment.
            [[nodiscard]]
            std::wstring close_stack()
                {
                if (m_stacks.size() == 0)
                    {
                    return L"";
                    }
                // close the <span> generating commands in the current set of {}
                std::wstring closeStr;
                if (m_stacks.back() != 0)
                    {
                    closeStr.reserve(m_stacks.back() * 7 /*</span>*/);
                    for (size_t i = 0; i < m_stacks.back(); ++i)
                        {
                        closeStr += L"</span>";
                        }
                    }
                m_stacks.pop_back();
                return closeStr;
                }

            /// @brief Add another `<span>` generating tag for the current environment.
            void add_command() noexcept
                {
                if (m_stacks.size())
                    {
                    ++(m_stacks.back());
                    }
                }

          private:
            std::vector<size_t> m_stacks;
            };

        command_stacks m_command_stacks;

        static const rtf_to_text_symbol_table RTF_TO_TEXT_TABLE;
        static const rtf_to_html_symbol_table RTF_TO_HTML_TABLE;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__RTF_EXTRACT_H__
