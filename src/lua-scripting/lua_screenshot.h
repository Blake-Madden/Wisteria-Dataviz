/********************************************************************************
 * Copyright (c) 2005-2026 Blake Madden
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Blake Madden - initial implementation
 ********************************************************************************/

#ifndef WISTERIA_LUASCREENSHOT_H
#define WISTERIA_LUASCREENSHOT_H

#include "lua.hpp"

// NOLINTBEGIN(readability-identifier-length)
// NOLINTBEGIN(readability-implicit-bool-conversion)
// cppcheck-suppress-begin functionConst

/// @private
namespace LuaScripting
    {
    int ConvertImage(lua_State* L);
    int SnapScreenshot(lua_State* L);
    int SnapScreenshotWithAnnotation(lua_State* L);
    int CropScreenshot(lua_State* L);
    int SnapScreenshotOfTextWindow(lua_State* L);
    int SnapScreenshotOfRibbon(lua_State* L);
    int SnapScreenshotOfBackstage(lua_State* L);
    int SnapScreenshotOfListControl(lua_State* L);
    int SnapScreenshotOfDialogWithPropertyGrid(lua_State* L);
    int HighlightScreenshot(lua_State* L);
    int ShowSortListDlg(lua_State* L);
    int CloseSortListDlg(lua_State* L);
    int ShowListExportDlg(lua_State* L);
    int CloseListExportDlg(lua_State* L);
    int CloseListViewItemDlg(lua_State* L);
    int ShowListViewItemDlg(lua_State* L);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    static const luaL_Reg ScreenshotLib[] = {
        { "ConvertImage", ConvertImage },
        { "SnapScreenshot", SnapScreenshot },
        { "SnapScreenshotWithAnnotation", SnapScreenshotWithAnnotation },
        { "CropScreenshot", CropScreenshot },
        { "SnapScreenshotOfTextWindow", SnapScreenshotOfTextWindow },
        { "SnapScreenshotOfRibbon", SnapScreenshotOfRibbon },
        { "SnapScreenshotOfBackstage", SnapScreenshotOfBackstage },
        { "SnapScreenshotOfListControl", SnapScreenshotOfListControl },
        { "SnapScreenshotOfDialogWithPropertyGrid", SnapScreenshotOfDialogWithPropertyGrid },
        { "HighlightScreenshot", HighlightScreenshot },
        { "ShowSortListDlg", ShowSortListDlg },
        { "CloseSortListDlg", CloseSortListDlg },
        { "ShowListExportDlg", ShowListExportDlg },
        { "CloseListExportDlg", CloseListExportDlg },
        { "CloseListViewItemDlg", CloseListViewItemDlg },
        { "ShowListViewItemDlg", ShowListViewItemDlg },
        { nullptr, nullptr }
    };
    } // namespace LuaScripting

// cppcheck-suppress-end functionConst
// NOLINTEND(readability-implicit-bool-conversion)
// NOLINTEND(readability-identifier-length)

#endif // WISTERIA_LUASCREENSHOT_H
