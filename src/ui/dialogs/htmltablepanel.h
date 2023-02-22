/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WXHTML_PANEL_H__
#define __WXHTML_PANEL_H__

#include "../controls/htmltablewin.h"

namespace Wisteria::UI
    {
    /// @brief Dialog to dislay a HtmlTableWindow.
    class HtmlTablePanel : public wxWindow
        {
    public:
        /// @brief Constructor.
        /// @param parent The parent window.
        /// @param id The dialog's ID.
        /// @param bkColor The dialog's background color.
        HtmlTablePanel(wxWindow *parent, wxWindowID id, const wxColour bkColor);
        /// @private
        HtmlTablePanel() = delete;
        /// @private
        HtmlTablePanel(const HtmlTablePanel& that) = delete;
        /// @private
        HtmlTablePanel& operator=(const HtmlTablePanel& that) = delete;
        /// @returns The HTML table window.
        [[nodiscard]]
        HtmlTableWindow* GetHtmlWindow() noexcept
            { return m_htmlWindow; }
    private:
        void OnButtonClick(wxCommandEvent& event);
        HtmlTableWindow* m_htmlWindow{ nullptr };
        };
    }

/** @}*/

#endif //__WXHTML_PANEL_H__
