/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef SEARCH_PANEL_H
#define SEARCH_PANEL_H

#include <wx/srchctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Combines a @c wxSearchCtrl with find next, find previous,
    ///     and options button buddies.
    class SearchPanel final : public wxWindow
        {
      public:
        /// @brief Constructor.
        /// @param parent The parent window that handles the search events.
        ///     The parent should handle @c wxEVT_FIND_NEXT to process this control's events.
        /// @param id The control's ID.
        SearchPanel(wxWindow* parent, wxWindowID id);
        /// @private
        SearchPanel() = delete;
        /// @private
        SearchPanel(const SearchPanel&) = delete;
        /// @private
        SearchPanel& operator=(const SearchPanel&) = delete;

        /// @brief Sets the string to search for.
        /// @param value The text to search for.
        void SetFindString(const wxString& value) { m_search->SetValue(value); }

        /// @brief Sets whether whole-word searching should be used.
        /// @param wholeWord @c true to enable whole-word searching.
        void SetWholeWordSearch(const bool wholeWord) { m_wholeWordItem->Check(wholeWord); }

        /// @brief Sets whether case-sensitive searching should be used.
        /// @param matchCase @c true to enable case-sensitive searching.
        void SetMatchCase(const bool matchCase) { m_matchCaseItem->Check(matchCase); }

        /// @brief Sets the control's background color.
        /// @param color The color to use.
        /// @returns @c true upon success.
        bool SetBackgroundColour(const wxColour& color) final;

        /// @brief Gives the control the focus.
        void Activate();
        /// @brief Issues a search event.
        /// @param event The event.
        /// @note The control's parent should handle @c wxEVT_COMMAND_FIND to trap
        ///     and process the results of this call.
        void OnSearch(const wxCommandEvent& event);

      private:
        enum ControlIDs
            {
            ID_SEARCH_TEXT_ENTRY = wxID_HIGHEST,
            ID_SEARCH_NEXT,
            ID_SEARCH_PREVIOUS
            };

        void OnSearchButton(const wxCommandEvent& event);

        wxSearchCtrl* m_search{ nullptr };
        wxMenuItem* m_matchCaseItem{ nullptr };
        wxMenuItem* m_wholeWordItem{ nullptr };
        wxArrayString m_previousSearches;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // SEARCH_PANEL_H
