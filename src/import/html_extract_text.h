/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef HTML_EXTRACT_TEXT_H
#define HTML_EXTRACT_TEXT_H

#include "extract_text.h"
#include <algorithm>
#include <map>
#include <regex>
#include <set>
#include <vector>

/// @brief Helper classes for HTML parsing.
namespace html_utilities
    {
    /** @brief Class to convert a character to its symbol equivalent
            (for the notorious Symbol font).*/
    class symbol_font_table
        {
      public:
        /// @private
        symbol_font_table();
        /** Finds a letter's symbol equivalent and returns it.
                If there is no symbol equivalent, then the original letter is returned.
            @param letter The letter to find.
            @returns The symbol equivalent of the letter,
                or the letter if no equivalent is found.\n
                For example, 'S' will return 'Σ'.*/
        [[nodiscard]]
        wchar_t find(const wchar_t letter) const;

      private:
        std::unordered_map<wchar_t, wchar_t> m_symbol_table;
        };

    /** @brief Class to convert an HTML entity (e.g., "&amp;") to its literal value.*/
    class html_entity_table
        {
      public:
        /// @private
        html_entity_table();

        /** @returns The unicode value of an entity, or '?' if not valid.
            @param html_entity The entity to look up.*/
        [[nodiscard]]
        wchar_t operator[](const std::wstring_view html_entity) const
            {
            return find(html_entity);
            }

        /** @brief Searches for an entity.
            @returns The unicode value of an entity, or '?' if not valid.
            @param html_entity The entity to look up.
            @note This function first must do a case sensitive search because some HTML entities
                are case sensitive (e.g., "Dagger" and "dagger" result in different values).
                Of course, before XHTML, most HTML was very liberal with casing,
                so if a case sensitive search fails, then a case insensitive search is performed.
                In this case, whatever the HTML author's intention for something like "&SIGMA;"
                may be misinterpreted (should it be a lowercase or uppercase sigma symbol?)--
                the price you pay for sloppy HTML.*/
        [[nodiscard]]
        wchar_t find(const std::wstring_view html_entity) const;

      private:
        std::unordered_map<std::wstring_view, wchar_t> m_table;
        };

    /** @brief Functor that accepts a block of script text and
            returns the links in it, one-by-one.
        @details Links will be absolute URLs inside double quotes.*/
    class javascript_hyperlink_parse
        {
      public:
        /// @private
        javascript_hyperlink_parse() = default;

        /** @brief Constructor.
            @param js_text The JavaScript to analyze.
            @param length The length of js_text.*/
        javascript_hyperlink_parse(const wchar_t* js_text, const size_t length) noexcept
            : m_js_text_start(js_text), m_js_text_end(js_text + length)
            {
            }

        /** @brief Sets the JavaScript to analyze.
            @param js_text The JavaScript to analyze.
            @param length The length of js_text.*/
        void set(const wchar_t* js_text, const size_t length) noexcept
            {
            m_js_text_start = js_text;
            m_js_text_end = (js_text + length);
            m_current_hyperlink_length = 0;
            }

        /** @brief Main function that returns the next link in the file.
            @returns A pointer to the next link, or null when there are no more links.*/
        const wchar_t* operator()();

        /// @returns The last parsed link from @c operator().
        /// @note Will be @c nullptr if no more links were found.
        [[nodiscard]]
        const wchar_t* get_current_link() const noexcept
            {
            return m_js_text_start;
            }

        /** @returns The length of the current hyperlink.*/
        [[nodiscard]]
        inline size_t get_current_hyperlink_length() const noexcept
            {
            return m_current_hyperlink_length;
            }

        /** @brief Parses JS sections from an HTML block and searches for cookie setting code.\n
                The cookie values will then be extracted and returned.
            @param htmlText The HTML text to parse.
            @returns The cookie values set within JS script blocks (if any).*/
        [[nodiscard]]
        static std::wstring get_cookies(std::wstring_view htmlText);

        /// @private
        static const std::wstring_view HTML_SCRIPT;
        /// @private
        static const std::wstring_view HTML_SCRIPT_WITH_ANGLE;
        /// @private
        static const std::wstring_view HTML_SCRIPT_END;

      private:
        const wchar_t* m_js_text_start{ nullptr };
        const wchar_t* m_js_text_end{ nullptr };
        size_t m_current_hyperlink_length{ 0 };
        };

    /** @brief Functor that accepts a block of HTML text and
            returns the image links in it, one-by-one.*/
    class html_image_parse
        {
      public:
        /** @brief Constructor.
            @param html_text The HTML text to analyze.
            @param length The length of html_text.*/
        html_image_parse(const wchar_t* html_text, const size_t length) noexcept
            : m_html_text(html_text), m_html_text_end(html_text + length)
            {
            }

        /** @brief Main function that returns the next link in the file.
            @returns either the next link or null when there are no more links.*/
        [[nodiscard]]
        const wchar_t* operator()();

        /** @returns The length of the current hyperlink.*/
        [[nodiscard]]
        inline size_t get_current_hyperlink_length() const noexcept
            {
            return m_current_hyperlink_length;
            }

      private:
        const wchar_t* m_html_text{ nullptr };
        const wchar_t* m_html_text_end{ nullptr };
        size_t m_current_hyperlink_length{ 0 };
        };

    /** @brief Functor that accepts a block of HTML text and
            returns the links in it, one-by-one.
        @details Links will include base hrefs, link hrefs, anchor hrefs,
            image map hrefs, image links, JavaScript links, and HTTP redirects.*/
    class html_hyperlink_parse
        {
      public:
        /** @brief Constructor.
            @param html_text The HTML text to analyze.
            @param length The length of html_text.*/
        html_hyperlink_parse(const wchar_t* html_text, const size_t length) noexcept;

        /// @returns The base web directory.
        [[nodiscard]]
        const wchar_t* get_base_url() const noexcept
            {
            return m_base;
            }

        /// @returns The length of the base web directory.
        [[nodiscard]]
        size_t get_base_url_length() const noexcept
            {
            return m_base_length;
            }

        /** @brief Main function that returns the next link in the file.
            @returns Either the next link or null when there are no more links.*/
        [[nodiscard]]
        const wchar_t* operator()();

        /** @returns The length of the current hyperlink.*/
        [[nodiscard]]
        inline size_t get_current_hyperlink_length() const noexcept
            {
            return m_current_hyperlink_length;
            }

        /** @brief Specifies whether or not image links should be reviewed.
            @param include Set to false to skip image links, true to include them.*/
        inline void include_image_links(const bool include) noexcept
            {
            m_include_image_links = include;
            }

        /** @returns @c true if the current hyperlink is pointing to an image.*/
        [[nodiscard]]
        inline bool is_current_link_an_image() const noexcept
            {
            return m_current_link_is_image;
            }

        /** @returns @c true if the current hyperlink is pointing to a JavaScript.*/
        [[nodiscard]]
        inline bool is_current_link_a_javascript() const noexcept
            {
            return m_current_link_is_javascript;
            }

        /** @brief Finds the end of an url by searching for the first illegal character.
            @param text the HTML text to analyze.
            @returns The valid end of the URL (i.e., the first illegal character).*/
        [[nodiscard]]
        static const wchar_t* find_url_end(const wchar_t* text) noexcept
            {
            while (text && is_legal_url_character(text[0]))
                {
                ++text;
                }
            return text;
            }

        /** Indicates whether or not character is legal to be in an URL.
                If false, then that character should be hex encoded.
            @param ch The character to review.
            @returns Whether the character is illegal and needs to be encoded.*/
        [[nodiscard]]
        inline constexpr static bool is_legal_url_character(const wchar_t ch)
            {
            return (((ch >= 0x41) && (ch <= 0x5A)) || // A-Z
                    ((ch >= 0x61) && (ch <= 0x7A)) || // a-1
                    ((ch >= 0x30) && (ch <= 0x39)) || // 0-9
                    (ch == L'.') || (ch == 0x24) ||   // $
                    (ch == L'&') || (ch == 0x2B) ||   // +
                    (ch == 0x2C) ||                   // ,
                    (ch == 0x2F) ||                   // /
                    (ch == 0x3A) ||                   // :
                    (ch == 0x3B) ||                   // ;
                    (ch == 0x3D) ||                   // =
                    (ch == 0x3F) ||                   // ?
                    (ch == 0x40)                      // @
            );
            }

      private:
        const wchar_t* m_html_text{ nullptr };
        const wchar_t* m_html_text_end{ nullptr };
        size_t m_current_hyperlink_length{ 0 };
        const wchar_t* m_base{ nullptr };
        size_t m_base_length{ 0 };
        bool m_include_image_links{ true };
        bool m_current_link_is_image{ false };
        bool m_current_link_is_javascript{ false };
        bool m_inside_of_script_section{ false };
        javascript_hyperlink_parse m_javascript_hyperlink_parse;
        };

    /** @brief Wrapper class to generically handle hyperlink parsing for either
            JavaScript or HTML files.*/
    class hyperlink_parse
        {
      public:
        /// @brief How to parse a hyperlink.
        enum class hyperlink_parse_method
            {
            /// @brief The source being parsed is HTML.
            html,
            /// @brief The source being parsed is a `<script>` section.
            script
            };

        /** @brief Constructor to initialize parser.
            @param html_text The text to parse.
            @param length The length of the text being parsed.
            @param method How to parse the text. Either html or script.*/
        hyperlink_parse(const wchar_t* html_text, const size_t length,
                        const hyperlink_parse_method method) noexcept
            : m_html_hyperlink_parse(html_text, length),
              m_javascript_hyperlink_parse(html_text, length), m_method(method)
            {
            }

        /** @brief Main function that returns the next link in the file.
            @returns A pointer to the next link, or null when there are no more links.*/
        [[nodiscard]]
        const wchar_t* operator()()
            {
            return (get_parse_method() == hyperlink_parse_method::html) ?
                       m_html_hyperlink_parse() :
                       m_javascript_hyperlink_parse();
            }

        /** @returns The HTML parser.*/
        [[nodiscard]]
        html_hyperlink_parse get_html_parser() const noexcept
            {
            return m_html_hyperlink_parse;
            }

        /** @returns The JavaScript parser.*/
        [[nodiscard]]
        javascript_hyperlink_parse get_script_parser() const noexcept
            {
            return m_javascript_hyperlink_parse;
            }

        /** @returns The parsing method, either html or script.*/
        [[nodiscard]]
        hyperlink_parse_method get_parse_method() const noexcept
            {
            return m_method;
            }

        /** @returns the length of the current hyperlink.*/
        [[nodiscard]]
        inline size_t get_current_hyperlink_length() const noexcept
            {
            return (get_parse_method() == hyperlink_parse_method::html) ?
                       m_html_hyperlink_parse.get_current_hyperlink_length() :
                       m_javascript_hyperlink_parse.get_current_hyperlink_length();
            }

      private:
        html_hyperlink_parse m_html_hyperlink_parse;
        javascript_hyperlink_parse m_javascript_hyperlink_parse;
        hyperlink_parse_method m_method{ hyperlink_parse_method::html };
        };

    /** @brief Class to format a given filepath into a full URL,
            using a base URL as the starting point.*/
    class html_url_format
        {
      public:
        /** @brief Constructor, which accepts the base URL to format any relative links with.
            @param root_url The base directory to format the URLs to.*/
        explicit html_url_format(std::wstring_view root_url);
        /** @brief Main interface.
            @param path The filepath to format.
            @param is_image Whether or not the filepath is to an image.\n
                This is needed when formatting a full URL to an image from a PHP request.\n
                @c false is recommended.
            @returns The fully-formed file path.*/
        const wchar_t* operator()(std::wstring_view path, const bool is_image);

        /** @returns The domain of the base URL.\n
                For example, a base URL of "http://www.business.yahoo.com"
                will return "yahoo.com".*/
        [[nodiscard]]
        const std::wstring& get_root_domain() const noexcept
            {
            return m_root_domain;
            }

        /** @returns The subdomain of the base URL.\n
                For example, a base URL of "http://www.business.yahoo.com"
                will return "business.yahoo.com".*/
        [[nodiscard]]
        const std::wstring& get_root_subdomain() const noexcept
            {
            return m_root_subdomain;
            }

        /** @returns The full domain of the base URL (domain, subdomain, and protocol).*/
        [[nodiscard]]
        const std::wstring& get_root_full_domain() const noexcept
            {
            return m_root_full_domain;
            }

        /** @returns The domain of the current URL.\n
                For example, a base URL of "http://www.business.yahoo.com"
                will return "yahoo.com".*/
        [[nodiscard]]
        const std::wstring& get_domain() const noexcept
            {
            return m_current_domain;
            }

        /** @returns The subdomain of the current URL.\n
                For example, a base URL of "http://www.business.yahoo.com"
                will return "business.yahoo.com".*/
        [[nodiscard]]
        const std::wstring& get_subdomain() const noexcept
            {
            return m_current_subdomain;
            }

        /** @returns The full domain of the current URL (domain, subdomain, and protocol).*/
        [[nodiscard]]
        const std::wstring& get_full_domain() const noexcept
            {
            return m_current_full_domain;
            }

        /** @returns The subdomain and folder structure of the current URL.*/
        [[nodiscard]]
        std::wstring get_directory_path();

        /** @returns Whether or not URL starts with a server protocol (e.g., "http").
                If not, then it is a relative URL.
            @param url The url to analyze.*/
        [[nodiscard]]
        inline static bool is_absolute_url(std::wstring_view url) noexcept
            {
            return (string_util::strnicmp<wchar_t>(url.data(), L"http:", 5) == 0 ||
                    string_util::strnicmp<wchar_t>(url.data(), L"https:", 6) == 0 ||
                    string_util::strnicmp<wchar_t>(url.data(), L"www.", 4) == 0 ||
                    string_util::strnicmp<wchar_t>(url.data(), L"ftp:", 4) == 0 ||
                    string_util::strnicmp<wchar_t>(url.data(), L"ftps:", 5) == 0);
            }

        /** @returns Whether or not this URL has a PHP query command.*/
        [[nodiscard]]
        bool has_query() const noexcept
            {
            return m_query != std::wstring::npos;
            }

        /** @brief Looks for "image=" in the PHP command.
            @details Example, "www.mypage.com?image=hi.jpg&loc=location" would return "hi.jpg".
            @param url The url to analyze.
            @returns The image name.*/
        [[nodiscard]]
        static std::wstring parse_image_name_from_url(std::wstring_view url);
        /** @returns The top-level domain (e.g., .com or .org) from an url.
            @param url The url to analyze.*/
        [[nodiscard]]
        static std::wstring parse_top_level_domain_from_url(std::wstring_view url);
        /** @returns Whether an URL is just a domain and not a subfolder or file.
            @param url The url to analyze.*/
        [[nodiscard]]
        static bool is_url_top_level_domain(std::wstring_view url);

      protected:
        /** @returns The position of the top level direction in an URL, as well as
                the position of the query comment in it (if there is one).
                Also will add a slash to the URL if need.
            @param[in,out] url The URL to parse.
            @param[out] query_position The position in the URL of the query
                (e.g., PHP command).*/
        [[nodiscard]]
        static size_t find_last_directory(std::wstring& url, size_t& query_position);
        /** @brief Parses an URL into a full domain (domain, subdomain, and protocol),
                a domain, and subdomain.
            @param url The URL to parse.
            @param[out] full_domain The full domain (domain, subdomain, and protocol)
                of the URL.
            @param[out] domain The domain of the URL.
            @param[out] subdomain The subdomain of the URL.*/
        static void parse_domain(const std::wstring& url, std::wstring& full_domain,
                                 std::wstring& domain, std::wstring& subdomain);

      private:
        // info about the original starting URL
        std::wstring m_root_url;
        std::wstring m_root_full_domain;
        std::wstring m_root_domain;
        std::wstring m_root_subdomain;
        std::wstring m_image_name;
        size_t m_last_slash{ 0 };
        size_t m_query{ 0 };
        // the current url
        std::wstring m_current_url;
        std::wstring m_current_subdomain;
        std::wstring m_current_full_domain;
        std::wstring m_current_domain;
        };

    /** @brief Class to strip hyperlinks from an HTML stream. The rest of the HTML's
            format is preserved.*/
    class html_strip_hyperlinks : public lily_of_the_valley::extract_text
        {
      public:
        /** @brief Main interface for extracting plain text from an HTML buffer.
            @param html_text The HTML text to strip.
            @param text_length The length of the HTML text.
            @returns The HTML stream with the hyperlinks removed from it.*/
        [[nodiscard]]
        const wchar_t* operator()(const wchar_t* html_text, const size_t text_length);
        };

    /// @returns @c true if character is not safe to use in an URL.
    /// @param character The character to review.
    [[nodiscard]]
    inline constexpr bool is_unsafe_uri_char(const wchar_t character)
        {
        return (character > 127 /*extended ASCII*/ ||
                character < 33 /*control characters and space*/);
        }
    } // namespace html_utilities

namespace lily_of_the_valley
    {
    /** @brief Class to extract text from an HTML stream.
        @par Example:
        @code
        // insert your own HTML file here
        std::ifstream fs("PatientReport.htm", std::ios::in|std::ios::binary|std::ios::ate);
        if (fs.is_open())
            {
            // read an HTML file into a char* buffer first...
            size_t fileSize = fs.tellg();
            char* fileContents = new char[fileSize+1];
            std::unique_ptr<char> deleteBuffer(fileContents);
            std::memset(fileContents, 0, fileSize+1);
            fs.seekg(0, std::ios::beg);
            fs.read(fileContents, fileSize);

            // ...then convert it to a Unicode buffer.
            // Note that you should use the specified character set
            // in the HTML file (usually UTF-8) when loading this file into a wchar_t buffer.
            // Here we are using the standard mbstowcs() function which assumes the current
            // system's character set; however, it is recommended to parse the character set
            // from the HTML file (with parse_charset()) and use a
            // system-dependent function (e.g., MultiByteToWideChar() on Win32)
            // to properly convert the char buffer to Unicode using that character set.
            wchar_t* convertedFileContents = new wchar_t[fileSize+1];
            std::unique_ptr<wchar_t> deleteConvertedBuffer(convertedFileContents);
            const size_t convertedFileSize =
                std::mbstowcs(convertedFileContents, fileContents, fileSize);

            // convert the Unicode HTML data into raw text
            lily_of_the_valley::html_extract_text htmlExtract;
            htmlExtract(convertedFileContents, convertedFileSize, true, false);
            // The raw text from the file is now in a Unicode buffer.
            // This buffer can be accessed from get_filtered_text() and its length
            // from get_filtered_text_length(). Call these to copy the text into
            // a wide string.
            std::wstring fileText(htmlExtract.get_filtered_text(),
                                  htmlExtract.get_filtered_text_length());
            }
        @endcode
    */
    class html_extract_text : public extract_text
        {
      public:
        /** @brief Main interface for extracting plain text from an HTML buffer.
            @param html_text The HTML text to strip.
            @param text_length The length of the HTML text.
            @param include_outer_text Whether text outside the first and last `<>`
                should be included. Recommended @c true.
            @param preserve_newlines Whether embedded newlines should be included
                in the output. If @c false, then they will be replaced with spaces,
                which is the default for HTML renderers. Recommended @c false.
            @returns The plain text from the HTML stream.*/
        const wchar_t* operator()(const wchar_t* html_text, const size_t text_length,
                                  const bool include_outer_text, const bool preserve_newlines);

        /** @returns The title from the metadata file or stream.
            @note Must be called after calling operator() or read_meta_data()
                (depending on which parser you are using).*/
        [[nodiscard]]
        const std::wstring& get_title() const noexcept
            {
            return m_title;
            }

        /** @returns The subject from the metadata file or stream.
            @note Must be called after calling operator() or read_meta_data()
                (depending on which parser you are using).*/
        [[nodiscard]]
        const std::wstring& get_subject() const noexcept
            {
            return m_subject;
            }

        /** @returns The description from the metadata file or stream.
            @note Must be called after calling operator() or read_meta_data()
                (depending on which parser you are using).*/
        [[nodiscard]]
        const std::wstring& get_description() const noexcept
            {
            return m_description;
            }

        /** @returns The author from the metadata file or stream.
            @note Must be called after calling operator() or read_meta_data()
                (depending on which parser you are using).*/
        [[nodiscard]]
        const std::wstring& get_author() const noexcept
            {
            return m_author;
            }

        /** @returns The keywords from the metadata file or stream.
            @note Must be called after calling operator() or read_meta_data()
                (depending on which parser you are using).*/
        [[nodiscard]]
        const std::wstring& get_keywords() const noexcept
            {
            return m_keywords;
            }

        /** Compares (case insensitively) raw HTML text with an element constant to see
                if the current element that we are on is the one we are looking for.
            @param text The current position in the HTML buffer that we are examining.
            @param element The element that we are comparing against the current position.
            @param accept_self_terminating_elements Whether we should match elements that
                close themselves (i.e., don't have a matching "</[element]>",
                but rather end where it is declared).\n
                For example, "<br />" is a self-terminating element.\n
                You would set this to @c false if you only want to read text in between
                opening and closing tags.\n
                @c false is recommended for most cases.
            @returns @c true if the current position matches the element.
            @note Be sure to skip the starting '<' first.*/
        [[nodiscard]]
        static bool compare_element(const wchar_t* text, std::wstring_view element,
                                    const bool accept_self_terminating_elements);
        /** Compares (case sensitively) raw HTML text with an element constant to see
                if the current element that we are on is the one we are looking for.
                Be sure to skip the starting '<' first.
            @param text The current position in the HTML buffer that we are examining.
            @param element The element that we are comparing against the current position.
            @param accept_self_terminating_elements Whether we should match elements that
                close themselves (i.e., don't have a matching "</[element]>",
                but rather end where it is declared).\n
                For example, "<br />" is a self-terminating element.\n
                You would set this to @c false if you only want to read
                text in between opening and closing tags.\n
                @c false is recommended for most cases.
            @returns @c true if the current position matches the element.
            @note This function is case sensitive, so it should only be used for XML
                or strict HTML 4.0.*/
        [[nodiscard]]
        static bool compare_element_case_sensitive(const wchar_t* text, std::wstring_view element,
                                                   const bool accept_self_terminating_elements);
        /** @returns The current element that a stream is on. (This assumes that you have
                already skipped the leading < symbol.)
            @note The returned string view will wrap @c text and will share the same lifetime.
            @param text The HTML stream to analyze.
            @param accept_self_terminating_elements Whether to analyze element such as "<br />".\n
                @c true is recommended for most cases.*/
        [[nodiscard]]
        static string_util::case_insensitive_wstring_view
        get_element_name(const wchar_t* text, const bool accept_self_terminating_elements);
        /** @returns The body of an HTML buffer.
            @param text The HTML stream to parse.*/
        [[nodiscard]]
        static std::wstring get_body(const std::wstring_view& text);
        /** @returns The CSS style section from an HTML buffer.
            @param text The HTML stream to parse.*/
        [[nodiscard]]
        static std::wstring get_style_section(const std::wstring_view& text);
        /** @returns The matching > to a <, or null if not found.
            @param text The HTML stream to analyze.*/
        [[nodiscard]]
        static const wchar_t* find_close_tag(const wchar_t* text);
        /** @brief Reads the value inside an element as a string.
            @details For example, this would read "My title" from "<H1>My title</H1>".
            @param html_text The start of the element.
            @param html_end How far into the HTML buffer we should read.
            @param element The element that we are looking at.
            @returns The string inside the element.
            @note Returned string may need to be decoded by another html_extract_text object.*/
        [[nodiscard]]
        static std::wstring_view read_element_as_string(const wchar_t* html_text,
                                                        const wchar_t* html_end,
                                                        std::wstring_view element);
        /** @brief Searches for a tag inside an element and returns its value
                (or empty string if not found).
            @param text The start of the element section.
            @param tag The inner tag to search for (e.g., "bgcolor").
            @param allowQuotedTags Set this parameter to @c true for tags that are inside
                quotes (e.g., style values). For example, to read "bold" as the value for
                "font-weight" from "<span style="font-weight: bold;">",
                this should be set to @c true.
            @param allowSpacesInValue Whether there can be a spaces in the tag's value.
                Usually you would only see that with complex strings values, such as a font name.\n
                @c false is recommended for most cases.
            @returns The pointer to the tag value and its length.
                Returns null and length of zero on failure.*/
        [[nodiscard]]
        static std::pair<const wchar_t*, size_t>
        read_attribute(const wchar_t* text, std::wstring_view tag, const bool allowQuotedTags,
                       const bool allowSpacesInValue);
        /** @brief Same as read_attribute(), except it returns a `std::wstring`
                instead of a raw pointer.
            @param text The start of the element section.
            @param attribute The inner tag to search for (e.g., "bgcolor").
            @param allowQuotedTags Set this parameter to @c true for tags that are
                inside quotes (e.g., style values).\n
                For example, to read "bold" from "<span style="font-weight: bold;">",
                this would need to be @c true. Usually this would be @c false.
            @param allowSpacesInValue Whether there can be a spaces in the tag's value.
                Usually you would only see that with complex strings values,
                such as a font name. @c  false is recommended for most cases.
            @returns The tag value as a string, or empty string on failure.*/
        [[nodiscard]]
        static std::wstring
        read_attribute_as_string(const wchar_t* text, std::wstring_view attribute,
                                 const bool allowQuotedTags, const bool allowSpacesInValue);
        /** @brief Same as read_attribute(), except it returns the value as a long.
            @param text The start of the element section.
            @param attribute The inner tag to search for (e.g., "score").
            @param allowQuotedTags Set this parameter to true for tags that are
                inside quotes (e.g., style values).\n
                For example, to read "bold" from "<span style="font-weight: bold;">",
                this would need to be @c true. Usually this would be @c false.
            @returns The attribute value as a double, or zero on failure.*/
        [[nodiscard]]
        static long read_attribute_as_long(const wchar_t* text, std::wstring_view attribute,
                                           const bool allowQuotedTags);
        /** @brief Searches a buffer range for an element (e.g., "<h1>").
            @returns The pointer to the next element, or null if not found.
            @param sectionStart The start of the HTML buffer.
            @param sectionEnd The end of the HTML buffer.
            @param elementTag The name of the element (e.g., "table") to find.
            @param accept_self_terminating_elements @c true to accept tags such as "<br />".\n
                Usually should be @c true, use @c false here if searching for
                opening and closing elements with content in them.\n
                @c true is recommended for most cases.*/
        [[nodiscard]]
        static const wchar_t* find_element(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                           std::wstring_view elementTag,
                                           const bool accept_self_terminating_elements);
        /** @c brief Searches a buffer range for an element's matching close (e.g., "</h1>").
            @details If the search starts on the starting element, then it will search for
                the matching close tag (i.e., it will skip inner elements that have the same name
                and go to the correct closing element).
            @returns The pointer to the closing element, or null if not found.
            @param sectionStart The start of the HTML buffer.
            @param sectionEnd The end of the buffer.
            @param elementTag The element (e.g., "h1") whose respective ending element
                that we are looking for.*/
        [[nodiscard]]
        static const wchar_t* find_closing_element(const wchar_t* sectionStart,
                                                   const wchar_t* sectionEnd,
                                                   std::wstring_view elementTag);
        /** @brief Searches for an attribute inside an element.
            @returns The position of the attribute (or null if not found).
            @param text The start of the element section.
            @param tag The inner tag to search for (e.g., "bgcolor").
            @param allowQuotedTags Set this parameter to true for tags that are inside quotes
                (e.g., style values like "font-weight", as in "<span style="font-weight: bold;">").
                To find "font-weight" inside the style tag, this parameter should be @c true.\n
                Usually this would be @c false.
            @note If the position is currently on a '<' and you are searching for its matching '>',
                then you should set @c sectionStart to one character after the '<'.*/
        [[nodiscard]]
        static const wchar_t* find_tag(const wchar_t* text, std::wstring_view tag,
                                       const bool allowQuotedTags);
        /** @brief Searches a buffer range for a bookmark (e.g., "<a name="citation" />").
            @param sectionStart The start of the HTML buffer.
            @param sectionEnd The end of the HTML buffer.
            @returns The start of the element (i.e., "<a name="citation" /></> and the
                bookmark name (e.g., "citation").*/
        [[nodiscard]]
        static std::pair<const wchar_t*, std::wstring> find_bookmark(const wchar_t* sectionStart,
                                                                     const wchar_t* sectionEnd);
        /** @brief Searches a buffer range for an ID (e.g., "<div id="citation" />").
            @param[in,out] htmlText The the HTML buffer. Its "suffix" will be trimmed past
                the last read ID section. Will be empty if no more IDs could be found.
            @returns The ID name (e.g., "citation").*/
        [[nodiscard]]
        static std::wstring find_id(std::wstring_view& htmlText);
        /** @brief Searches for a single character in a string, but ensures that
                it is not inside a pair of double or single quotes.
            @details This is specifically tailored for " and ' type of quotes used
                for HTML attributes.
            @param string The string to search in.
            @param ch The character to search for.
            @returns The pointer of where the character is, or null if not found.*/
        [[nodiscard]]
        static const wchar_t* strchr_not_quoted(const wchar_t* string, const wchar_t ch) noexcept;
        /** @brief Searches for substring in string (case-insensitive), but ensures that
                what you are searching for is not in quotes.
            @details This is specifically tailored for " and ' type of quotes used
                for HTML attributes.
            @param string The string to search in.
            @param stringSize The length of string to search within.
            @param strSearch The substring to search for.
            @param strSearchSize The length of @c strSearch.
            @returns The pointer of where the character is, or null if not found.*/
        [[nodiscard]]
        static const wchar_t* stristr_not_quoted(const wchar_t* string, const size_t stringSize,
                                                 const wchar_t* strSearch,
                                                 const size_t strSearchSize) noexcept;
        /** @returns The charset from the meta section of an HTML buffer.
            @param pageContent The meta section to analyze.
            @param length The length of @c pageContent.*/
        [[nodiscard]]
        static std::string parse_charset(const char* pageContent, const size_t length);

        /// @brief Whether text from `<noscript>` blocks should be included in the results.
        /// @details These are using warning messages that scripting should be enabled
        ///     and are recommended to not be included.
        /// @param include @c true to include this sort of text.
        void include_no_script_sections(const bool include) noexcept
            {
            m_includeNoScriptSections = include;
            }

        /// @private
        static const html_utilities::symbol_font_table SYMBOL_FONT_TABLE;
        /// @private
        static const html_utilities::html_entity_table HTML_TABLE_LOOKUP;

      protected:
        /** @brief Converts a section of HTML text that is using Symbol font into the actual
                symbols that it is meant to display.
            @param symbolFontText The text to convert.
            @return The converted text.*/
        [[nodiscard]]
        static std::wstring convert_symbol_font_section(const std::wstring_view& symbolFontText);
        /** @brief Parses raw (HTML) text and loads its filtered content into the buffer.
            @param text The text to parse.
            @param textSize The size of the text being parsed.*/
        void parse_raw_text(const wchar_t* text, size_t textSize);

        /// @brief Resets the metadata (e.g., title, subject, etc.).
        void reset_meta_data() noexcept
            {
            m_subject.clear();
            m_title.clear();
            m_description.clear();
            m_author.clear();
            m_keywords.clear();
            }

        /// @private
        bool m_includeNoScriptSections{ false };

        /// @private
        size_t m_is_in_preformatted_text_block_stack{ 0 };
        /// @private
        size_t m_superscript_stack{ 0 };
        /// @private
        size_t m_subscript_stack{ 0 };
        /// @private
        std::wstring m_title;
        /// @private
        std::wstring m_subject;
        /// @private
        std::wstring m_description;
        /// @private
        std::wstring m_author;
        /// @private
        std::wstring m_keywords;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // HTML_EXTRACT_TEXT_H
