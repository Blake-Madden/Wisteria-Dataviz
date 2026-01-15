/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PARENT_BLOCKER_H
#define WISTERIA_PARENT_BLOCKER_H

#include <wx/window.h>

/** @brief Temporarily prevents a window from propagating its event to its parent.
    @details This is useful for when a parent sends an event to a child, but the child
        doesn't have a handler for that event. In that situation, the event will
        go back to the parent and cause an infinite loop, so this will prevent that.*/
class ParentEventBlocker
    {
  public:
    /** @brief Constructor/
        @param window The window whose event propagation should be blocked temporarily.*/
    explicit ParentEventBlocker(wxWindow* window)
        : m_window(window), m_style(window->GetExtraStyle())
        {

        m_window->SetExtraStyle(m_style | wxWS_EX_BLOCK_EVENTS);
        }

    /// @private
    ~ParentEventBlocker() { m_window->SetExtraStyle(m_style); }

    /// @private
    ParentEventBlocker(const ParentEventBlocker&) = delete;
    /// @private
    ParentEventBlocker& operator=(const ParentEventBlocker&) = delete;

  private:
    wxWindow* m_window{ nullptr };
    long m_style{ 0 };
    };

    /** @}*/

#endif // WISTERIA_PARENT_BLOCKER_H
