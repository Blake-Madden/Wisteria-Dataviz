/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2020
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __TAG_FILTER_H__
#define __TAG_FILTER_H__

#include <vector>
#include <map>
#include "extract_text.h"

namespace lily_of_the_valley
    {
    /// @brief Tag pairs to mark text to be excluded.
    class text_filter_tag
        {
    public:
        /** @brief Constructor.
            @param start_tag The opening tag.
            @param end_tag The closing tag.*/
        text_filter_tag(const std::wstring& start_tag, const std::wstring& end_tag) :
            m_start_tag(start_tag), m_end_tag(end_tag),
            m_tags_identical(start_tag == m_end_tag)
            {}
        /// @private
        text_filter_tag(std::wstring&& start_tag, std::wstring&& end_tag) :
            m_start_tag(std::move(start_tag)), m_end_tag(std::move(end_tag)),
            m_tags_identical(start_tag == m_end_tag)
            {}
        /// @brief Determines if a block of text is an opening tag.
        /// @param text The block of text to review.
        /// @returns @c true if @c text is the start of a filtered section.
        [[nodiscard]]
        bool operator==(const wchar_t* text) const noexcept
            { return std::wcsncmp(m_start_tag.c_str(), text, m_start_tag.length()) == 0; }
        /// @returns The opening tag.
        [[nodiscard]]
        const std::wstring& get_start_tag() const noexcept
            { return m_start_tag; }
        /// @returns The closing tag.
        [[nodiscard]]
        const std::wstring& get_end_tag() const noexcept
            { return m_end_tag; }
        /// @returns @c true if the opening and closing tags are the same.
        [[nodiscard]]
        bool tags_are_identical() const noexcept
            { return m_tags_identical; }
    private:
        std::wstring m_start_tag;
        std::wstring m_end_tag;
        bool m_tags_identical{ false };
        };

    /// @brief Filters tagged sections out of a block of text.
    class tag_filter : public extract_text
        {
    public:
        /** @brief Filters blocks of text from a text stream based on pairs of tags
                which denote the blocks to ignore.
            @param text The text stream to filter.
            @param length The length of @c text.
            @returns The text stream, with any sections within the filter tags removed.
            @sa add_filter_tag().*/
        [[nodiscard]]
        const wchar_t* operator()(const wchar_t* text, const size_t length)
            {
            if (text == nullptr || length == 0)
                {
                set_filtered_text_length(0);
                return nullptr;
                }
            if (!allocate_text_buffer(length))
                {
                set_filtered_text_length(0);
                return nullptr;
                }

            // marks the beginning and size of the sections to copy over
            std::map<size_t,size_t> sectionsToIncludeMarkers;
            size_t currentInclusionStart{ 0 };
            std::vector<text_filter_tag>::const_iterator exclusion_tag_pos;

            for (size_t i = 0; i < length; /*handled in loop*/)
                {
                exclusion_tag_pos =
                    std::find(m_text_filter_tags.cbegin(), m_text_filter_tags.cend(), text + i);
                if (exclusion_tag_pos != m_text_filter_tags.cend())
                    {
                    sectionsToIncludeMarkers.insert(
                        std::make_pair(currentInclusionStart, i - currentInclusionStart));
                    // find the end tag for the current exclusion block
                    const wchar_t* const endTag = exclusion_tag_pos->tags_are_identical() ?
                        std::wcsstr(text + i + exclusion_tag_pos->get_start_tag().length(),
                        exclusion_tag_pos->get_end_tag().c_str()) :
                        // start/end tags are different, so overlapping tags are
                        // OK here and should be checked for
                        string_util::find_matching_close_tag(
                            text + i + exclusion_tag_pos->get_start_tag().length(),
                        exclusion_tag_pos->get_start_tag().c_str(),
                        exclusion_tag_pos->get_end_tag().c_str());
                    // if not found then exclude the rest of the text;
                    // otherwise, move to the end of that tag and start the next
                    // inclusion block from there.
                    if (endTag == nullptr || endTag >= text + length)
                        { break; }
                    else
                        {
                        currentInclusionStart = i =
                            (endTag - text) + exclusion_tag_pos->get_end_tag().length();
                        }
                    }
                else
                    { ++i; }
                }
            // add the rest of the text block to be included
            sectionsToIncludeMarkers.insert(
                std::make_pair(currentInclusionStart, length - currentInclusionStart));

            // copy the included text blocks into the buffer
            for (auto inclusionsPos = sectionsToIncludeMarkers.cbegin();
                inclusionsPos != sectionsToIncludeMarkers.cend();
                ++inclusionsPos)
                { add_characters(text+inclusionsPos->first, inclusionsPos->second); }

            return get_filtered_text();
            }
        /// @brief Adds a set of filtering tags.
        /// @param tags The pair of tags to use for blocking out sections of text.
        void add_filter_tag(const text_filter_tag& tags) noexcept
            { return m_text_filter_tags.push_back(tags); }
        /// @private
        void add_filter_tag(text_filter_tag&& tags) noexcept
            { return m_text_filter_tags.emplace_back(tags); }
        /// @brief Removes the filter tags.
        void clear_tags() noexcept
            { m_text_filter_tags.clear(); }
    private:
        std::vector<text_filter_tag> m_text_filter_tags;
        };
    }

/** @}*/

#endif //__TAG_FILTER_H__
