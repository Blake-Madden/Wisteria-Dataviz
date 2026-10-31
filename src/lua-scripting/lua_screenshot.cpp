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

#include "lua_screenshot.h"
#include "../import/html_encode.h"
#include "../ui/app.h"
#include "../ui/dialogs/gridexportdlg.h"
#include "../ui/dialogs/listctrlitemviewdlg.h"
#include "../ui/dialogs/listctrlsortdlg.h"
#include "../util/screenshot.h"
#include "lua_debug.h"
#include <algorithm>
#include <utility>
#include <vector>
#include <wx/webview.h>

namespace LuaScripting
    {
    // NOLINTBEGIN
    static Wisteria::UI::GridExportDlg* LuaListExportDlg{ nullptr };
    static Wisteria::UI::ListCtrlItemViewDlg* LuaListViewItemDlg{ nullptr };
    static Wisteria::UI::ListCtrlSortDlg* LuaListCtrlSortDlg{ nullptr };

    // NOLINTEND

    //-------------------------------------------------------------
    int ConvertImage(lua_State* L)
        {
        if (!VerifyParameterCount(L, 3, __func__))
            {
            return 0;
            }

        lua_pushboolean(L, static_cast<int>(Screenshot::ConvertImageToPng(
                               wxString{ luaL_checkstring(L, 1), wxConvUTF8 },
                               wxSize(lua_tonumber(L, 2), lua_tonumber(L, 3)), true)));
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshot(lua_State* L)
        {
        // wait for any changes in the UI to finish updating
        ::wxSleep(2);
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }

        int startWindowToHighlight = wxID_ANY, endWindowToHighlight = wxID_ANY,
            cutOffWindow = wxID_ANY;
        if (lua_gettop(L) > 1)
            {
            const wxWindowID wId = lua_tonumber(L, 2);
            if (wId != wxID_ANY)
                {
                auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(wId);
                if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                    {
                    startWindowToHighlight = idPos->second;
                    }
                else
                    {
                    startWindowToHighlight = lua_tonumber(L, 2);
                    }
                }
            }
        if (lua_gettop(L) > 2)
            {
            const wxWindowID wId = lua_tonumber(L, 3);
            if (wId != wxID_ANY)
                {
                auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(wId);
                if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                    {
                    endWindowToHighlight = idPos->second;
                    }
                else
                    {
                    endWindowToHighlight = lua_tonumber(L, 3);
                    }
                }
            }
        if (lua_gettop(L) > 3)
            {
            const wxWindowID wId = lua_tonumber(L, 4);
            if (wId != wxID_ANY)
                {
                auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(wId);
                if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                    {
                    cutOffWindow = idPos->second;
                    }
                else
                    {
                    cutOffWindow = lua_tonumber(L, 4);
                    }
                }
            }
        lua_pushboolean(L, static_cast<int>(Screenshot::SaveScreenshot(
                               wxString{ luaL_checkstring(L, 1), wxConvUTF8 },
                               startWindowToHighlight, endWindowToHighlight, cutOffWindow)));
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshotWithAnnotation(lua_State* L)
        {
        ::wxSleep(2);
        if (!VerifyParameterCount(L, 3, __func__))
            {
            return 0;
            }

        int startWindowToHighlight = wxID_ANY, endWindowToHighlight = wxID_ANY;

        auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 3));
        if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
            {
            startWindowToHighlight = idPos->second;
            }
        else
            {
            startWindowToHighlight = lua_tonumber(L, 3);
            }

        if (lua_gettop(L) > 3)
            {
            auto endIdPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 4));
            if (endIdPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                {
                endWindowToHighlight = endIdPos->second;
                }
            else
                {
                endWindowToHighlight = lua_tonumber(L, 4);
                }
            }

        lua_pushboolean(L, static_cast<int>(Screenshot::SaveScreenshot(
                               wxString{ luaL_checkstring(L, 1), wxConvUTF8 },
                               wxString{ luaL_checkstring(L, 2), wxConvUTF8 },
                               startWindowToHighlight, endWindowToHighlight)));
        return 1;
        }

    //-------------------------------------------------------------
    int CropScreenshot(lua_State* L)
        {
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }

        wxCoord x{ wxDefaultCoord }, y{ wxDefaultCoord };

        if (lua_gettop(L) >= 2)
            {
            x = lua_tonumber(L, 2);
            }
        if (lua_gettop(L) >= 3)
            {
            y = lua_tonumber(L, 3);
            }

        if (Screenshot::CropScreenshot(wxString{ luaL_checkstring(L, 1), wxConvUTF8 }, x, y))
            {
            lua_pushboolean(L, 1);
            return 1;
            }

        lua_pushboolean(L, 0);
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshotOfTextWindow(lua_State* L)
        {
        if (!VerifyParameterCount(L, 2, __func__))
            {
            return 0;
            }

        wxWindowID windowId = lua_tonumber(L, 2);
        if (const auto windowMappedId =
                Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 2));
            windowMappedId != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
            {
            windowId = windowMappedId->second;
            }

        wxWindow* windowToCapture = Screenshot::GetActiveDialogOrFrame();
        if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
            {
            windowToCapture = wxTopLevelWindows.GetLast()->GetData();
            }
        if (windowToCapture == nullptr)
            {
            lua_pushboolean(L, 0);
            return 1;
            }
        if (windowToCapture->GetId() != windowId ||
            !windowToCapture->IsKindOf(CLASSINFO(wxWebView)))
            {
            wxWindow* foundWindow = windowToCapture->FindWindow(windowId);
            if (foundWindow != nullptr && foundWindow->IsKindOf(CLASSINFO(wxWebView)))
                {
                windowToCapture = foundWindow;
                }
            else
                {
                lua_pushboolean(L, 0);
                return 1;
                }
            }

        auto* webView = dynamic_cast<wxWebView*>(windowToCapture);
        wxASSERT(webView);
        if (webView == nullptr)
            {
            lua_pushboolean(L, 0);
            return 1;
            }

        // the window's content is loaded into the webview asynchronously and stays
        // hidden until that finishes; wait for it so the screenshot doesn't race and
        // catch it still blank
        for (int waitAttempts = 0; !webView->IsShown() && waitAttempts < 8; ++waitAttempts)
            {
            wxMilliSleep(500);
            wxTheApp->Yield();
            }

        std::vector<std::pair<long, long>> highlightPoints;
        if (lua_gettop(L) > 3)
            {
            // Search for the strings to highlight and store their positions in the page's
            // text. Normally searched for in the order that they appear in the text, but
            // falls back to searching from the start of the page if a string turns out to
            // be positioned earlier than the previous one (e.g., duplicate/reused text
            // elsewhere on the page).
            long previousEnd{ 0 };
            for (long i = 4; i <= lua_gettop(L); ++i)
                {
                const wxString contentToFind{ luaL_checkstring(L, i), wxConvUTF8 };
                long foundStart{ -1 }, foundEnd{ -1 };
                if (Screenshot::FindWebViewTextRange(webView, contentToFind, previousEnd,
                                                     foundStart, foundEnd) ||
                    (previousEnd > 0 && Screenshot::FindWebViewTextRange(webView, contentToFind, 0,
                                                                         foundStart, foundEnd)))
                    {
                    highlightPoints.emplace_back(foundStart, foundEnd);
                    previousEnd = std::max(previousEnd, foundEnd);
                    }
                else
                    {
                    lua_Debug ar{};
                    wxString lineInfo;
                    if (lua_getstack(L, 1, &ar) != 0 && lua_getinfo(L, "l", &ar) != 0 &&
                        ar.currentline > 0)
                        {
                        // quneiform-suppress-begin
                        lineInfo = wxString::Format(_(L" (chunk line #%d)"), ar.currentline);
                        // quneiform-suppress-end
                        }
                    DebugPrint(wxString::Format(
                        // TRANSLATORS: %s are formatting tags and
                        // should stay wrapped around "Warning"
                        _(L"⚠️%sWarning%s: unable to find \"%s\" in web view.") + lineInfo,
                        L"<span class='warning' style='font-weight:bold;'>", L"</span>",
                        // the excerpt is from the document being searched and may itself
                        // contain '<'/'>'/'&', which would otherwise get parsed as markup
                        // and swallow the rest of this HTML-rendered debug message
                        wxString{ lily_of_the_valley::html_encode_text::simple_encode(
                            wxString{ contentToFind }
                                .Truncate(10)
                                .append(contentToFind.length() > 10 ? wxString{ _DT(L"...") } :
                                                                      wxString{})
                                .ToStdWstring()) }));
                    DebugPrint(wxString{});
                    }
                }
            }

        lua_pushboolean(L, static_cast<int>(Screenshot::SaveScreenshotOfWebView(
                               wxString{ luaL_checkstring(L, 1), wxConvUTF8 }, windowId,
                               int_to_bool(lua_toboolean(L, 3)), highlightPoints)));
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshotOfRibbon(lua_State* L)
        {
        ::wxSleep(2);
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }

        int pageToSelect{ 0 }, firstButtonBarID{ -1 }, lastButtonBarID{ -1 };
        if (lua_gettop(L) >= 2)
            {
            auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 2));
            if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                {
                pageToSelect = idPos->second;
                }
            else
                {
                pageToSelect = lua_tonumber(L, 2);
                }
            }
        if (lua_gettop(L) >= 3)
            {
            auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 3));
            if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                {
                firstButtonBarID = idPos->second;
                }
            else
                {
                firstButtonBarID = lua_tonumber(L, 3);
                }
            }
        if (lua_gettop(L) >= 4)
            {
            auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 4));
            if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                {
                lastButtonBarID = idPos->second;
                }
            else
                {
                lastButtonBarID = lua_tonumber(L, 4);
                }
            }
        lua_pushboolean(L, static_cast<int>(Screenshot::SaveScreenshotOfRibbon(
                               wxString{ luaL_checkstring(L, 1), wxConvUTF8 }, pageToSelect,
                               firstButtonBarID, lastButtonBarID)));
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshotOfBackstage(lua_State* L)
        {
        ::wxSleep(2);
        if (!VerifyParameterCount(L, 2, __func__))
            {
            return 0;
            }

        wxWindowID sectionId = lua_tonumber(L, 2);
        if (const auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(sectionId);
            idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
            {
            sectionId = idPos->second;
            }
        wxWindowID controlToHighlight{ wxID_ANY };
        if (lua_gettop(L) >= 3)
            {
            controlToHighlight = lua_tonumber(L, 3);
            if (const auto idPos =
                    Wisteria::UI::BaseApp::GetDynamicIdMap().find(controlToHighlight);
                idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                {
                controlToHighlight = idPos->second;
                }
            }
        lua_pushboolean(
            L, static_cast<int>(Screenshot::SaveScreenshotOfBackstage(
                   wxString{ luaL_checkstring(L, 1), wxConvUTF8 }, sectionId, controlToHighlight)));
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshotOfListControl(lua_State* L)
        {
        ::wxSleep(2);
        if (!VerifyParameterCount(L, 2, __func__))
            {
            return 0;
            }

        int startRow{ -1 }, endRow{ -1 }, startColumn{ -1 }, endColumn{ -1 }, cutOffRow{ -1 };
        if (lua_gettop(L) >= 3)
            {
            startRow = lua_tonumber(L, 3);
            }
        if (lua_gettop(L) >= 4)
            {
            startColumn = lua_tonumber(L, 4);
            }
        if (lua_gettop(L) >= 5)
            {
            endRow = lua_tonumber(L, 5);
            }
        if (lua_gettop(L) >= 6)
            {
            endColumn = lua_tonumber(L, 6);
            }
        if (lua_gettop(L) >= 7)
            {
            cutOffRow = lua_tonumber(L, 7);
            }
        wxWindowID windowId = lua_tonumber(L, 2);
        if (const auto windowMappedId =
                Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 2));
            windowMappedId != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
            {
            windowId = windowMappedId->second;
            }
        lua_pushboolean(
            L, static_cast<int>(Screenshot::SaveScreenshotOfListControl(
                   wxString{ luaL_checkstring(L, 1), wxConvUTF8 }, windowId,
                   // make zero-indexed
                   startRow == -1 ? -1 : startRow - 1, endRow == -1 ? -1 : endRow - 1,
                   startColumn == -1 ? -1 : startColumn - 1, endColumn == -1 ? -1 : endColumn - 1,
                   cutOffRow == -1 ? -1 : cutOffRow - 1)));
        return 1;
        }

    //-------------------------------------------------------------
    int SnapScreenshotOfDialogWithPropertyGrid(lua_State* L)
        {
        ::wxSleep(2);
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }

        int propGridId = wxID_ANY;
        wxString propertyStart, propertyEnd;
        if (lua_gettop(L) > 1)
            {
            auto idPos = Wisteria::UI::BaseApp::GetDynamicIdMap().find(lua_tonumber(L, 2));
            if (idPos != Wisteria::UI::BaseApp::GetDynamicIdMap().cend())
                {
                propGridId = idPos->second;
                }
            else
                {
                propGridId = lua_tonumber(L, 2);
                }
            }
        if (lua_gettop(L) > 2)
            {
            propertyStart = wxString{ luaL_checkstring(L, 3), wxConvUTF8 };
            }
        if (lua_gettop(L) > 3)
            {
            propertyEnd = wxString{ luaL_checkstring(L, 4), wxConvUTF8 };
            }
        lua_pushboolean(
            L, static_cast<int>(Screenshot::SaveScreenshotOfDialogWithPropertyGrid(
                   wxString{ luaL_checkstring(L, 1), wxConvUTF8 }, propGridId, propertyStart,
                   propertyEnd,
                   std::make_pair(((lua_gettop(L) > 4) ? int_to_bool(lua_toboolean(L, 5)) : false),
                                  ((lua_gettop(L) > 5) ? lua_tonumber(L, 6) : -1)))));
        return 1;
        }

    //-------------------------------------------------------------
    int HighlightScreenshot(lua_State* L)
        {
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }

        lua_pushboolean(L, static_cast<int>(Screenshot::HighlightItemInScreenshot(
                               wxString{ luaL_checkstring(L, 1), wxConvUTF8 },
                               wxPoint(lua_tonumber(L, 2), lua_tonumber(L, 3)),
                               wxPoint(lua_tonumber(L, 4), lua_tonumber(L, 5)))));
        return 1;
        }

    //-------------------------------------------------------------
    int ShowSortListDlg(lua_State* L)
        {
        wxArrayString columns;
        for (int i = 1; i < lua_gettop(L) + 1; ++i)
            {
            columns.Add(wxGetTranslation(wxString{ luaL_checkstring(L, i), wxConvUTF8 }));
            }
        if (LuaListCtrlSortDlg == nullptr)
            {
            LuaListCtrlSortDlg = new Wisteria::UI::ListCtrlSortDlg(
                Wisteria::UI::BaseApp::GetApp().GetMainFrame(), columns);
            }
        std::vector<std::pair<size_t, Wisteria::SortDirection>> sortInfo;
        sortInfo.reserve(columns.size());
        for (size_t i = 0; i < columns.size(); ++i)
            {
            sortInfo.emplace_back(i, Wisteria::SortDirection::SortAscending);
            }
        LuaListCtrlSortDlg->FillSortCriteria(sortInfo);
        LuaListCtrlSortDlg->Show();
        wxTheApp->Yield();
        return 0;
        }

    //-------------------------------------------------------------
    int CloseSortListDlg([[maybe_unused]] lua_State* L)
        {
        if (LuaListCtrlSortDlg != nullptr)
            {
            LuaListCtrlSortDlg->Destroy();
            LuaListCtrlSortDlg = nullptr;
            }
        wxTheApp->Yield();
        return 0;
        }

    //-------------------------------------------------------------
    int ShowListExportDlg(lua_State* L)
        {
        if (LuaListExportDlg == nullptr)
            {
            LuaListExportDlg = new Wisteria::UI::GridExportDlg(
                Wisteria::UI::BaseApp::GetApp().GetMainFrame(), lua_tonumber(L, 1),
                lua_tonumber(L, 2), Wisteria::UI::GridExportFormat::ExportHtml);
            }
        LuaListExportDlg->IncludeColumnHeaders(int_to_bool(lua_toboolean(L, 3)));
        LuaListExportDlg->ExportSelectedRowsOnly(int_to_bool(lua_toboolean(L, 4)));
        LuaListExportDlg->Show();
        wxTheApp->Yield();
        return 0;
        }

    //-------------------------------------------------------------
    int CloseListExportDlg([[maybe_unused]] lua_State* L)
        {
        if (LuaListExportDlg != nullptr)
            {
            LuaListExportDlg->Destroy();
            LuaListExportDlg = nullptr;
            }
        wxTheApp->Yield();
        return 0;
        }

    //-------------------------------------------------------------
    int CloseListViewItemDlg([[maybe_unused]] lua_State* L)
        {
        if (LuaListViewItemDlg != nullptr)
            {
            LuaListViewItemDlg->Destroy();
            LuaListViewItemDlg = nullptr;
            }
        wxTheApp->Yield();
        return 0;
        }

    //-------------------------------------------------------------
    int ShowListViewItemDlg(lua_State* L)
        {
        if (LuaListViewItemDlg != nullptr)
            {
            CloseListViewItemDlg(L);
            }
        LuaListViewItemDlg = new Wisteria::UI::ListCtrlItemViewDlg;
        for (int i = 1; i < lua_gettop(L); i += 2)
            {
            LuaListViewItemDlg->AddValue(
                wxGetTranslation(wxString{ luaL_checklstring(L, i, nullptr), wxConvUTF8 }),
                wxGetTranslation(wxString{ luaL_checklstring(L, i + 1, nullptr), wxConvUTF8 }));
            }
        LuaListViewItemDlg->Create(Wisteria::UI::BaseApp::GetApp().GetMainFrame());
        LuaListViewItemDlg->Show();
        wxTheApp->Yield();
        return 0;
        }
    } // namespace LuaScripting
