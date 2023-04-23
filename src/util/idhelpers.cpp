///////////////////////////////////////////////////////////////////////////////
// Name:        idhelpers.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "idhelpers.h"

wxWindowID IdRangeLock::m_startingId{ wxID_HIGHEST };

//------------------------------------------------------------------
void MenuEnableItem(wxMenu* menu, const wxWindowID id, const bool enable)
    {
    if (menu == nullptr)
        { return; }
    const wxMenuItemList& menuItems = menu->GetMenuItems();
    for (wxMenuItemList::compatibility_iterator node = menuItems.GetFirst();
         node; node = node->GetNext())
        {
        wxMenuItem* item = node->GetData();
        if (item && item->GetId() == id)
            { item->Enable(enable); }
        // go through any items on the submenu too (if there is one)
        if (item && item->IsSubMenu() )
            { MenuEnableItem(item->GetSubMenu(), id, enable); }
        }
    }

//------------------------------------------------------------------
void MenuBarEnableAll(wxMenuBar* menuBar, const wxWindowID id, const bool enable)
    {
    if (menuBar == nullptr)
        { return; }
    const auto menuCount = menuBar->GetMenuCount();
    for (size_t i = 0; i < menuCount; ++i)
        {
        wxMenu* menu = menuBar->GetMenu(i);
        if (menu)
            { MenuEnableItem(menu, id, enable); }
        }
    }
