///////////////////////////////////////////////////////////////////////////////
// Name:        accessibilityoptionspanel.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef ACCESSIBILITY_OPTIONS_PANEL_H
#define ACCESSIBILITY_OPTIONS_PANEL_H

#include "../../base/graphitems.h"
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief A panel for editing ARIA accessibility options for a graph item.*/
    class AccessibilityOptionsPanel : public wxPanel
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.*/
        explicit AccessibilityOptionsPanel(wxWindow* parent);

        /** @brief Populates the controls from a graph item's accessibility info.
            @param item The item to load from.*/
        void LoadFromItem(const GraphItems::GraphItemBase& item);

        /** @brief Writes the controls' values back to a graph item.
            @param item The item to apply the values to.*/
        void ApplyToItem(GraphItems::GraphItemBase& item) const;

      private:
        void OnAutoCheck(wxCommandEvent& event);

        wxStaticText* m_ariaLabelLabel{ nullptr };
        wxStaticText* m_roleLabel{ nullptr };
        wxTextCtrl* m_ariaLabelCtrl{ nullptr };
        wxChoice* m_roleChoice{ nullptr };
        wxCheckBox* m_ariaHiddenCheck{ nullptr };
        wxCheckBox* m_autoAccessibilityCheck{ nullptr };
        };
    } // namespace Wisteria::UI

#endif // ACCESSIBILITY_OPTIONS_PANEL_H
