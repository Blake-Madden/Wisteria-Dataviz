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

#include "lua_debug.h"
#include "../reporting/reportbuilder.h"
#include "../ui/app.h"
#include "lua_interface.h"
#include <algorithm>
#include <wx/stopwatch.h>

// NOLINTBEGIN(readability-identifier-length)
// NOLINTBEGIN(readability-implicit-bool-conversion)

namespace LuaScripting
    {
    //-------------------------------------------------------------
    wxColour LoadColor(wxString colorStr)
        {
        wxColour color{ colorStr.MakeLower() };
        if (!color.IsOk())
            {
            auto foundColor = Wisteria::ReportBuilder::GetColorMap().find(colorStr.wc_str());
            if (foundColor != Wisteria::ReportBuilder::GetColorMap().cend())
                {
                color = Wisteria::Colors::ColorBrewer::GetColor(foundColor->second);
                }
            }
        return color;
        }

    //-------------------------------------------------------------
    void LoadFontAttributes(lua_State* L, wxFont& font, wxColour& fontColor, bool calledFromObject)
        {
        int paramIndex = (calledFromObject ? 2 : 1);
        // name, point size, weight, color (as a string)
        wxString fontName{ luaL_checkstring(L, paramIndex++), wxConvUTF8 };
        if (fontName.CmpNoCase(_DT(L"MONOSPACE")) == 0)
            {
            fontName = Wisteria::GraphItems::Label::GetFirstAvailableMonospaceFont();
            }
        else if (fontName.CmpNoCase(_DT(L"CURSIVE")) == 0)
            {
            fontName = Wisteria::GraphItems::Label::GetFirstAvailableCursiveFont();
            }
        if (!fontName.empty())
            {
            font.SetFaceName(fontName);
            }

        if (lua_gettop(L) >= paramIndex)
            {
            const double pointSize{ luaL_checknumber(L, paramIndex++) };
            if (pointSize > 0)
                {
                font.SetFractionalPointSize(pointSize);
                }
            }

        if (lua_gettop(L) >= paramIndex)
            {
            const int fontWeight{ static_cast<int>(luaL_checkinteger(L, paramIndex++)) };
            if (fontWeight > 0)
                {
                font.SetWeight(static_cast<wxFontWeight>(fontWeight));
                }
            }

        if (lua_gettop(L) >= paramIndex)
            {
            fontColor = LoadColor(wxString{ luaL_checkstring(L, paramIndex++), wxConvUTF8 });
            }
        }

    //-------------------------------------------------------------
    bool VerifyParameterCount(lua_State* L, const int minParameterCount,
                              const wxString& functionName)
        {
        assert(L);
        assert(minParameterCount >= 0);
        if (lua_gettop(L) < minParameterCount)
            {
            wxMessageBox(wxString::Format(
                             // TRANSLATORS: %s is a function name that failed from a script
                             _(L"%s: invalid number of arguments.\n\n%d expected, %d provided."),
                             functionName, minParameterCount, lua_gettop(L)),
                         _(L"Script Error"), wxOK | wxICON_EXCLAMATION);
            return false;
            }

        return true;
        }

    //-------------------------------------------------------------
    void DebugPrint(const wxString& str)
        {
        auto* luaRunner = Wisteria::UI::BaseApp::GetApp().GetLuaRunner();
        if (luaRunner != nullptr)
            {
            luaRunner->DebugOutput(str);
            }
        wxTheApp->Yield();
        }

    //-------------------------------------------------------------
    int GetScriptFolder(lua_State* L)
        {
        const wxString& scriptFilePath =
            Wisteria::UI::BaseApp::GetApp().GetLuaRunner()->GetScriptFilePath();
        if (scriptFilePath.empty())
            {
            DebugPrint(wxString::Format( // TRANSLATORS: %s are highlighting tags and
                                         // should stay wrapped around "Error"
                _(L"❌%sError%s: call to %s returned empty because "
                  "the script has not been saved yet."),
                L"<span class='issue' style='font-weight:bold;'>", L"</span>", __func__));
            lua_pushstring(L, "");
            return 1;
            }
        lua_pushstring(L, wxFileName{ scriptFilePath }.GetPathWithSep().utf8_str());
        return 1;
        }

    //-------------------------------------------------------------
    int Print(lua_State* L)
        {
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }
        DebugPrint(wxString{ luaL_checkstring(L, 1), wxConvUTF8 });
        return 0;
        }

    //-------------------------------------------------------------
    int Clear([[maybe_unused]] lua_State* L)
        {
        auto* luaRunner = Wisteria::UI::BaseApp::GetApp().GetLuaRunner();
        if (luaRunner != nullptr)
            {
            luaRunner->DebugClear();
            }
        wxTheApp->Yield();
        return 0;
        }

    //-------------------------------------------------------------
    int Wait(lua_State* L)
        {
        if (!VerifyParameterCount(L, 1, __func__))
            {
            return 0;
            }
        constexpr lua_Integer MAX_SLEEP_INTERVAL{ 20 };
        const lua_Integer waitTime{ luaL_checkinteger(L, 1) };
        // sleep in short intervals so that the UI stays responsive and Stop takes effect
        const wxStopWatch stopWatch;
        while (!Wisteria::UI::LuaInterpreter::IsQuitRequested())
            {
            const lua_Integer remainingTime{ waitTime - stopWatch.Time() };
            if (remainingTime <= 0)
                {
                break;
                }
            wxMilliSleep(static_cast<unsigned long>(std::min(remainingTime, MAX_SLEEP_INTERVAL)));
            wxSafeYield(wxTheApp->GetTopWindow(), true);
            }
        return 0;
        }
    } // namespace LuaScripting

// NOLINTEND(readability-implicit-bool-conversion)
// NOLINTEND(readability-identifier-length)
