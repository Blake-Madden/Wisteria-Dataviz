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

#include "script_workbench_panel.h"
#include "../base/colorbrewer.h"
#include "../import/text_matrix.h"
#include "../import/text_preview.h"
#include "../lua/lua.h"
#include "../ui/app.h"
#include "../util/textstream.h"
#include <algorithm>
#include <array>
#include <set>
#include <utility>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <wx/srchctrl.h>
#include <wx/stc/minimap.h>

namespace Wisteria::UI
    {
    //-------------------------------------------------------
    // CodeEditor lines are 0-based; Lua reports 1-based line numbers
    std::set<int> ScriptWorkbenchPanel::ToLuaLines(const std::vector<int>& zeroBasedLines)
        {
        std::set<int> result;
        for (const int line : zeroBasedLines)
            {
            result.insert(line + 1);
            }
        return result;
        }

    //-------------------------------------------------------
    ScriptWorkbenchPanel::ScriptWorkbenchPanel(wxWindow* parent, wxWindowID id /*= wxID_ANY*/,
                                               wxString apiFilePrefix /*= wxString{}*/,
                                               wxString defaultScriptHeader /*= wxString{}*/)
        : wxPanel(parent, id), m_apiFilePrefix(std::move(apiFilePrefix)),
          m_defaultScriptHeader(std::move(defaultScriptHeader))
        {
        wxASSERT_MSG(BaseApp::GetApp().GetLuaRunner() != nullptr,
                     L"The application's Lua runner must be set before creating the workbench!");
        ImportAPI();
        CreateControls();
        BindEvents();
        BaseApp::GetApp().GetLuaRunner()->SetScriptWorkbench(this);
        }

    //-------------------------------------------------------
    ScriptWorkbenchPanel::~ScriptWorkbenchPanel()
        {
        if (auto* luaRunner = BaseApp::GetApp().GetLuaRunner(); luaRunner != nullptr)
            {
            luaRunner->SetScriptWorkbench(nullptr);
            }
        }

    //------------------------------------------------------
    void ScriptWorkbenchPanel::ImportAPI()
        {
        if (m_apiFilePrefix.empty())
            {
            return;
            }

        std::vector<std::vector<std::wstring>> apiStrings;

        lily_of_the_valley::standard_delimited_character_column tabbedColumn(
            lily_of_the_valley::text_column_delimited_character_parser{ L'\t' }, 1);
        lily_of_the_valley::standard_delimited_character_column semiColonColumn(
            lily_of_the_valley::text_column_delimited_character_parser{ L';' }, std::nullopt);
        lily_of_the_valley::text_row<std::wstring> row(std::nullopt);
        row.treat_consecutive_delimiters_as_one(true);
        row.add_column(tabbedColumn);
        row.add_column(semiColonColumn);
        row.allow_column_resizing(true);

        lily_of_the_valley::text_matrix<std::wstring> importer(&apiStrings);
        importer.add_row_definition(lily_of_the_valley::text_row<std::wstring>(1));
        importer.add_row_definition(row);

        lily_of_the_valley::text_preview preview;

        const auto loadApiFile =
            [&](const wxString& resourceName, std::vector<LibnameAndEntries>& target)
        {
            wxString libraryText;
            wxString libFilePath = BaseApp::GetApp().FindResourceFile(resourceName);
            if (!Wisteria::TextStream::ReadFile(libFilePath, libraryText))
                {
                return;
                }
            apiStrings.clear();
            const size_t rowCount = preview(libraryText, L'\t', true, false);
            if (rowCount == 0)
                {
                return;
                }
            apiStrings.resize(rowCount);
            importer.read(libraryText, rowCount, 2, true);

            for (auto& lib : apiStrings)
                {
                if (lib.empty())
                    {
                    continue;
                    }
                const std::wstring libName = lib.front();
                lib.erase(lib.begin(), lib.begin() + 1);
                CodeEditor::NameList nl;
                for (const auto& entry : lib)
                    {
                    nl.insert(entry);
                    }
                target.emplace_back(libName, nl);
                }
        };

        loadApiFile(m_apiFilePrefix + L"-classes.api", m_classes);
        loadApiFile(m_apiFilePrefix + L"-libraries.api", m_libraries);
        loadApiFile(m_apiFilePrefix + L"-enums.api", m_enums);
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::CreateControls()
        {
        // outer horizontal splitter: top = editor area, bottom = debug
        m_outerSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                               wxSP_LIVE_UPDATE | wxSP_3DSASH);
        m_outerSplitter->SetMinimumPaneSize(FromDIP(80));
        m_outerSplitter->SetSashGravity(1.0); // top (editor area) absorbs resize

        // sidebar splitter (top half): left = sidebar, right = editor + func browser
        m_sidebarSplitter = new wxSplitterWindow(m_outerSplitter, wxID_ANY, wxDefaultPosition,
                                                 wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH);
        m_sidebarSplitter->SetMinimumPaneSize(FromDIP(120));
        m_sidebarSplitter->SetSashGravity(0.0); // right (editor area) absorbs resize

        m_scriptSidebar = new SideBar(m_sidebarSplitter);
        m_scriptSidebar->InsertItem(0, _(L"Scripts"), m_scriptsFolderId, std::nullopt);
        BaseApp::GetApp().ApplyThemeToSideBar(m_scriptSidebar);

        // func browser splitter (right of sidebar): left = editor book, right = func browser
        m_funcBrowserSplitter = new wxSplitterWindow(m_sidebarSplitter, wxID_ANY, wxDefaultPosition,
                                                     wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH);
        m_funcBrowserSplitter->SetMinimumPaneSize(FromDIP(200));
        m_funcBrowserSplitter->SetSashGravity(1.0); // left (editor) absorbs resize

        m_editorBook = new wxSimplebook(m_funcBrowserSplitter, wxID_ANY);

        m_functionBrowser = new FunctionBrowserCtrl(m_funcBrowserSplitter, this);
        m_functionBrowser->AddCategory(_(L"Libraries"), 1000);
        m_functionBrowser->AddCategory(_(L"Classes"), 1001);
        m_functionBrowser->AddCategory(_(L"Enumerations"), 1002);
        for (const auto& theClass : m_classes)
            {
            m_functionBrowser->AddCategory(theClass.first, theClass.second, 1001);
            }
        for (const auto& theLib : m_libraries)
            {
            m_functionBrowser->AddCategory(theLib.first, theLib.second, 1000);
            }
        for (const auto& theLib : m_enums)
            {
            m_functionBrowser->AddCategory(theLib.first, theLib.second, 1002);
            }
        m_functionBrowser->FinalizeCategories();
        BaseApp::GetApp().ApplyThemeToSideBar(m_functionBrowser->GetSidebar());
        m_functionBrowser->Hide();

        // func browser starts hidden; the editor book occupies the entire right side
        m_funcBrowserSplitter->Initialize(m_editorBook);

        // sidebar | (editor + func browser)
        m_sidebarSplitter->SplitVertically(m_scriptSidebar, m_funcBrowserSplitter, FromDIP(200));

        // bottom half: tabbed "Output" (debug webview) and "Locals" (paused-frame variables)
        m_debugNotebook = new wxNotebook(m_outerSplitter, wxID_ANY);

        m_debugMessageWindow = wxWebView::New(m_debugNotebook, wxID_ANY);
#ifndef NDEBUG
        m_debugMessageWindow->EnableContextMenu(true);
        m_debugMessageWindow->EnableAccessToDevTools();
#else
        m_debugMessageWindow->EnableContextMenu(false);
#endif
        m_debugNotebook->AddPage(m_debugMessageWindow, _(L"Output"), true);

        m_localsWindow = new wxTreeListCtrl(m_debugNotebook, wxID_ANY);
        m_localsWindow->AppendColumn(_(L"Name"));
        m_localsWindow->AppendColumn(_(L"Value"));
        m_localsWindow->AppendColumn(_(L"Type"));
        m_debugNotebook->AddPage(m_localsWindow, _(L"Locals"), false);

        m_outerSplitter->SplitHorizontally(m_sidebarSplitter, m_debugNotebook, -FromDIP(225));

        auto* outerSizer = new wxBoxSizer(wxVERTICAL);
        outerSizer->Add(m_outerSplitter, wxSizerFlags{ 1 }.Expand());
        SetSizer(outerSizer);

        // seed the default untitled script (with the import header, marked clean)
        AddScriptEntry(_(L"(unnamed)"));

        DebugClear();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::BindEvents()
        {
        // search ctrl events (e.g., the function browser's search box)
        Bind(wxEVT_SEARCH,
             [this](wxCommandEvent& evt)
             {
                 auto* codeEditor = GetCurrentEditor();
                 if (codeEditor != nullptr)
                     {
                     codeEditor->FindNext(evt.GetString());
                     }
             });

        // find / replace dialog events
        Bind(wxEVT_FIND, &ScriptWorkbenchPanel::OnFindDialog, this);
        Bind(wxEVT_FIND_NEXT, &ScriptWorkbenchPanel::OnFindDialog, this);
        Bind(wxEVT_FIND_REPLACE, &ScriptWorkbenchPanel::OnFindDialog, this);
        Bind(wxEVT_FIND_REPLACE_ALL, &ScriptWorkbenchPanel::OnFindDialog, this);
        Bind(wxEVT_FIND_CLOSE, &ScriptWorkbenchPanel::OnFindDialog, this);

        Bind(
            wxEVT_CHAR_HOOK,
            [this](wxKeyEvent& event)
            {
                if (event.GetKeyCode() == WXK_F3)
                    {
                    wxFindDialogEvent evt{ wxEVT_FIND_NEXT };
                    evt.SetFindString(m_findData.GetFindString());
                    evt.SetFlags(m_findData.GetFlags());
                    OnFindDialog(evt);
                    }
                else if (event.GetKeyCode() == L'G' && event.ControlDown())
                    {
                    GotoLineDialog();
                    return;
                    }
                else if (event.GetKeyCode() == L'/' && event.ControlDown())
                    {
                    auto* editor = GetCurrentEditor();
                    if (editor != nullptr)
                        {
                        const int lineStart = editor->LineFromPosition(editor->GetSelectionStart());
                        const wxString lineText = editor->GetLine(lineStart).Trim(false);
                        if (lineText.starts_with(L"--"))
                            {
                            Uncomment();
                            }
                        else
                            {
                            Comment();
                            }
                        }
                    return;
                    }
                event.Skip(true);
            },
            wxID_ANY);

        // re-render the debug webview so its color-scheme CSS picks up the new theme
        Bind(wxEVT_SYS_COLOUR_CHANGED,
             [this](wxSysColourChangedEvent& event)
             {
                 RefreshDebugWindow();
                 // each editor re-themes itself asynchronously off this same system
                 // event, but its minimap only copies colors once (when created), so
                 // force the editor to re-theme now and re-sync its minimap in lockstep
                 for (const auto& entry : m_scripts)
                     {
                     if (entry.m_editor == nullptr || entry.m_miniMap == nullptr)
                         {
                         continue;
                         }
                     entry.m_editor->UpdateSystemThemeColors();
                     entry.m_miniMap->SetEdit(nullptr);
                     entry.m_miniMap->SetEdit(entry.m_editor);
                     }
                 event.Skip();
             });

        // sidebar selection swaps the visible editor
        Bind(wxEVT_SIDEBAR_CLICK, &ScriptWorkbenchPanel::OnSidebarClick, this);

        // lazily fetch a table's entries the first time it's expanded in the Locals window
        m_localsWindow->Bind(wxEVT_TREELIST_ITEM_EXPANDING,
                             &ScriptWorkbenchPanel::OnLocalsItemExpanding, this);
        }

    //-------------------------------------------------------
    CodeEditor* ScriptWorkbenchPanel::GetCurrentEditor() const
        {
        const int sel = m_editorBook->GetSelection();
        if (sel == wxNOT_FOUND)
            {
            return nullptr;
            }
        for (const auto& entry : m_scripts)
            {
            if (m_editorBook->FindPage(entry.m_page) == sel)
                {
                return entry.m_editor;
                }
            }
        return nullptr;
        }

    //-------------------------------------------------------
    CodeEditor* ScriptWorkbenchPanel::CreateLuaScript(wxWindow* parent)
        {
        auto* codeEditor = new CodeEditor(parent, wxSTC_LEX_LUA);
        codeEditor->Show(false);
        codeEditor->IncludeNumberMargin(true);
        codeEditor->IncludeFoldingMargin(true);
        codeEditor->IncludeBreakpointMargin(true);
        codeEditor->SetDefaultHeader(m_defaultScriptHeader);
        codeEditor->SetText(codeEditor->GetDefaultHeader());

        for (const auto& theClass : m_classes)
            {
            codeEditor->AddClass(theClass.first, theClass.second);
            }
        for (const auto& theLib : m_libraries)
            {
            codeEditor->AddLibrary(theLib.first, theLib.second);
            }
        for (const auto& theLib : m_enums)
            {
            codeEditor->AddLibrary(theLib.first, theLib.second);
            }
        codeEditor->Finalize();

        codeEditor->SetSelection(codeEditor->GetTextLength(), codeEditor->GetTextLength());
        // mark the seeded header as the save point so undo can't roll past it and
        // the buffer isn't considered dirty on creation
        codeEditor->EmptyUndoBuffer();
        codeEditor->SetSavePoint();
        codeEditor->SetModified(false);
        codeEditor->Show();

        return codeEditor;
        }

    //-------------------------------------------------------
    CodeEditor* ScriptWorkbenchPanel::AddScriptEntry(const wxString& label)
        {
        auto* page = new wxPanel(m_editorBook);
        auto* codeEditor = CreateLuaScript(page);
        auto* miniMap = new wxStyledTextCtrlMiniMap(page, codeEditor);
        auto* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(codeEditor, wxSizerFlags{ 1 }.Expand());
        sizer->Add(miniMap, wxSizerFlags{}.Expand());
        page->SetSizer(sizer);
        page->SetClientData(codeEditor);

        m_editorBook->AddPage(page, label, true);

        const wxWindowID subId = m_nextSidebarSubId++;
        m_scriptSidebar->InsertSubItemById(m_scriptsFolderId, label, subId, std::nullopt);
        m_scriptSidebar->GetFolder(0).Expand();
        m_scriptSidebar->Realize();
        m_scriptSidebar->SelectSubItemById(m_scriptsFolderId, subId, false, false);

        m_scripts.push_back(ScriptEntry{ subId, page, codeEditor, miniMap });

        codeEditor->Bind(wxEVT_STC_SAVEPOINTLEFT,
                         [this, codeEditor](wxStyledTextEvent&) { UpdateDirtyMark(codeEditor); });
        codeEditor->Bind(wxEVT_STC_SAVEPOINTREACHED,
                         [this, codeEditor](wxStyledTextEvent&) { UpdateDirtyMark(codeEditor); });

        codeEditor->SetFocus();
        return codeEditor;
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::UpdateDirtyMark(CodeEditor* editor)
        {
        for (const auto& entry : m_scripts)
            {
            if (entry.m_editor != editor)
                {
                continue;
                }
            const wxString& path = entry.m_editor->GetScriptFilePath();
            const wxString base = path.empty() ? _(L"(unnamed)") : wxFileName{ path }.GetName();
            const wxString label = entry.m_editor->GetModify() ? (base + L" *") : base;
            m_scriptSidebar->SetSubItemLabel(m_scriptsFolderId, entry.m_sidebarId, label);
            return;
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::NewScript() { AddScriptEntry(_(L"(unnamed)")); }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::OpenScriptFromFile(const wxString& path)
        {
        // if only the default empty/untitled script is open, replace it with the loaded file
        if (m_scripts.size() == 1)
            {
            auto* current = m_scripts.front().m_editor;
            if (current != nullptr && !current->GetModify() && current->GetScriptFilePath().empty())
                {
                // set the path before LoadFile so the SAVEPOINTREACHED handler can
                // derive the sidebar label from it
                current->SetScriptFilePath(path);
                current->LoadFile(path);
                current->SetSelection(current->GetTextLength(), current->GetTextLength());
                m_editorBook->SetPageText(0, wxFileName{ path }.GetName());
                current->SetFocus();
                return;
                }
            }

        // ...otherwise open as a new entry
        auto* editor = AddScriptEntry(wxFileName{ path }.GetName());
        if (editor != nullptr)
            {
            // set the path before LoadFile so the SAVEPOINTREACHED handler can
            // derive the sidebar label from it
            editor->SetScriptFilePath(path);
            editor->LoadFile(path);
            editor->SetSelection(editor->GetTextLength(), editor->GetTextLength());
            editor->SetFocus();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::SaveCurrentScript()
        {
        auto* codeEditor = GetCurrentEditor();
        if (codeEditor == nullptr || !codeEditor->Save())
            {
            return;
            }
        const int sel = m_editorBook->GetSelection();
        if (sel == wxNOT_FOUND)
            {
            return;
            }
        const wxString newLabel = wxFileName{ codeEditor->GetScriptFilePath() }.GetName();
        m_editorBook->SetPageText(sel, newLabel);

        for (const auto& entry : m_scripts)
            {
            if (entry.m_editor == codeEditor)
                {
                m_scriptSidebar->SetSubItemLabel(m_scriptsFolderId, entry.m_sidebarId, newLabel);
                break;
                }
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::ContinueScript()
        {
        if (!LuaInterpreter::IsPaused() || m_runningEditor == nullptr)
            {
            return;
            }
        auto* luaRunner = BaseApp::GetApp().GetLuaRunner();
        // re-sync in case breakpoints were toggled in the margin while paused
        luaRunner->SetBreakpointLines(ToLuaLines(m_runningEditor->GetBreakpointLines()));
        LuaInterpreter::ContinueExecution();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::RunCurrentScript()
        {
        if (LuaInterpreter::IsRunning())
            {
            return;
            }

        auto* luaRunner = BaseApp::GetApp().GetLuaRunner();
        auto* editor = GetCurrentEditor();
        if (editor == nullptr)
            {
            return;
            }
        editor->AnnotationClearAll();
        editor->ClearExecutionHighlight();

        const bool isSelectionRun = (editor->GetSelectionStart() != editor->GetSelectionEnd());
        // selection runs never pause on breakpoints
        luaRunner->SetBreakpointLines(isSelectionRun ? std::set<int>{} :
                                                       ToLuaLines(editor->GetBreakpointLines()));
        luaRunner->SetPauseStateChangedCallback([this](const int line)
                                                { OnLuaPauseStateChanged(line); });

        wxString errorMessage;
        m_isScriptRunning = true;
        m_runningEditor = editor;

        luaRunner->RunLuaCode(isSelectionRun ? editor->GetSelectedText() : editor->GetValue(),
                              editor->GetScriptFilePath(), errorMessage);

        editor->ClearExecutionHighlight();
        m_isScriptRunning = false;
        m_runningEditor = nullptr;

        if (errorMessage.empty())
            {
            return;
            }

        const int lineOffset =
            isSelectionRun ? editor->LineFromPosition(editor->GetSelectionStart()) : 0;

        long lineNumber{ 0 };
        const bool parsed = errorMessage.ToLong(&lineNumber);
        --lineNumber; // zero-indexed
        lineNumber += lineOffset;

        if (const auto foundPos = errorMessage.find(L':'); foundPos != wxString::npos)
            {
            errorMessage.erase(0, foundPos + 1);
            }

        wxMessageBox(
            wxString::Format(_(L"Line #%s: %s"), std::to_wstring(lineNumber + 1), errorMessage),
            _(L"Script Error"), wxOK | wxICON_EXCLAMATION);

        if (!parsed || lineNumber < 0 || lineNumber >= editor->GetLineCount())
            {
            return;
            }
        errorMessage.insert(0, _(L"Error:"));
        editor->GotoLine((editor->GetFirstVisibleLine() < lineNumber) ? lineNumber :
                                                                        lineNumber - 1);
        editor->AnnotationSetText(lineNumber, errorMessage);
        editor->AnnotationSetStyle(lineNumber, CodeEditor::ERROR_ANNOTATION_STYLE);

        // Scintilla doesn't update scroll width for annotations, do it manually
        const int width = editor->GetScrollWidth();
        const unsigned long indent = editor->GetLineIndentation(lineNumber) + FromDIP(3);
        const int widthAnn = editor->TextWidth(CodeEditor::ERROR_ANNOTATION_STYLE,
                                               errorMessage + wxString{ indent, L' ' });
        if (widthAnn > width)
            {
            editor->SetScrollWidth(widthAnn);
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::OnLuaPauseStateChanged(const int line)
        {
        // Force the ribbon's Run/Continue/Stop enabled-state to refresh immediately,
        // rather than waiting for the next idle-time wxUpdateUIEvent. This callback
        // fires from deep inside a nested Lua hook loop, so it can't be left to chance.
        BaseApp::GetApp().GetMainFrame()->UpdateWindowUI(wxUPDATE_UI_RECURSE);

        if (m_runningEditor == nullptr)
            {
            return;
            }

        if (line < 0)
            {
            m_runningEditor->ClearExecutionHighlight();
            ClearLocalsWindow();
            return;
            }

        // bring the workbench to the front in case it's hidden or behind other windows;
        // the paused script would otherwise be invisible with no obvious way to resume it
        BaseApp::GetApp().GetMainFrame()->ActivateScriptWorkbench();

        // jump to the paused tab (and its sidebar entry) so the highlight is
        // visible, even if a different script currently has focus
        for (const auto& entry : m_scripts)
            {
            if (entry.m_editor == m_runningEditor)
                {
                const int idx = m_editorBook->FindPage(entry.m_page);
                if (idx != wxNOT_FOUND && m_editorBook->GetSelection() != idx)
                    {
                    m_editorBook->SetSelection(idx);
                    }
                m_scriptSidebar->SelectSubItemById(m_scriptsFolderId, entry.m_sidebarId, false,
                                                   false);
                break;
                }
            }

        // Lua 1-based -> Scintilla 0-based
        m_runningEditor->HighlightExecutionLine(line - 1);
        DebugOutput(wxString::Format(_(L"⏸️Paused at breakpoint, line #%d"), line));

        PopulateLocalsWindow();
        if (!IsDebugWindowVisible())
            {
            ToggleDebugWindow();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::ClearLocalsWindow()
        {
        if (m_localsWindow != nullptr)
            {
            m_localsWindow->DeleteAllItems();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::PopulateLocalsWindow()
        {
        ClearLocalsWindow();
        for (const auto& local : BaseApp::GetApp().GetLuaRunner()->GetLocalVariables())
            {
            AddLocalsTreeItem(m_localsWindow->GetRootItem(), local);
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::AddLocalsTreeItem(wxTreeListItem parent,
                                                 const LuaInterpreter::LuaVariableInfo& info)
        {
        const wxTreeListItem item = m_localsWindow->AppendItem(parent, info.m_name);
        m_localsWindow->SetItemText(item, LOCALS_VALUE_COLUMN, info.m_value);
        m_localsWindow->SetItemText(item, LOCALS_TYPE_COLUMN, info.m_type);

        if (info.m_isExpandable)
            {
            m_localsWindow->SetItemData(item, new LocalsTableItemData(info.m_tableRef));
            // seed a placeholder child so the expand arrow renders; swapped out
            // for the table's real entries the first time it's expanded
            m_localsWindow->AppendItem(item, wxString{});
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::OnLocalsItemExpanding(wxTreeListEvent& event)
        {
        const wxTreeListItem item = event.GetItem();
        auto* data = dynamic_cast<LocalsTableItemData*>(m_localsWindow->GetItemData(item));
        if (data == nullptr || data->m_populated)
            {
            return;
            }
        data->m_populated = true;

        const wxTreeListItem placeholder = m_localsWindow->GetFirstChild(item);
        const auto entries = BaseApp::GetApp().GetLuaRunner()->GetTableEntries(data->m_tableRef);
        for (const auto& entry : entries)
            {
            AddLocalsTreeItem(item, entry);
            }

        // Drop the placeholder only after real entries are added.
        if (!entries.empty())
            {
            m_localsWindow->DeleteItem(placeholder);
            }
        else
            {
            m_localsWindow->SetItemText(placeholder, _(L"(empty)"));
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::StopScript()
        {
        LuaInterpreter::Quit();
        m_isScriptRunning = false;
        if (m_runningEditor != nullptr)
            {
            m_runningEditor->ClearExecutionHighlight();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::RestartInterpreter()
        {
        if (LuaInterpreter::IsRunning())
            {
            return;
            }
        BaseApp::GetApp().GetLuaRunner()->Restart();
        DebugOutput(_(L"Lua interpreter restarted; all global variables have been cleared."));
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::IsFunctionBrowserVisible() const noexcept
        {
        return (m_funcBrowserSplitter != nullptr && m_funcBrowserSplitter->IsSplit());
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::IsDebugWindowVisible() const noexcept
        {
        return (m_outerSplitter != nullptr && m_outerSplitter->IsSplit());
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::ToggleDebugWindow()
        {
        if (m_outerSplitter->IsSplit())
            {
            m_outerSplitter->Unsplit(m_debugNotebook);
            }
        else
            {
            m_debugNotebook->Show();
            m_outerSplitter->SplitHorizontally(m_sidebarSplitter, m_debugNotebook, -FromDIP(225));
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::UpdateTheme()
        {
        BaseApp::GetApp().ApplyThemeToSideBar(m_scriptSidebar);
        BaseApp::GetApp().ApplyThemeToSideBar(m_functionBrowser->GetSidebar());
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::ToggleFunctionBrowser()
        {
        if (m_funcBrowserSplitter->IsSplit())
            {
            m_funcBrowserSplitter->Unsplit(m_functionBrowser);
            }
        else
            {
            m_functionBrowser->Show();
            m_funcBrowserSplitter->SplitVertically(m_editorBook, m_functionBrowser, -FromDIP(500));
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::OnSidebarClick(wxCommandEvent& event)
        {
        const wxWindowID clickedId = event.GetInt();
        for (const auto& entry : m_scripts)
            {
            if (entry.m_sidebarId == clickedId)
                {
                const int idx = m_editorBook->FindPage(entry.m_page);
                if (idx != wxNOT_FOUND)
                    {
                    m_editorBook->SetSelection(idx);
                    if (entry.m_editor != nullptr)
                        {
                        entry.m_editor->SetFocus();
                        }
                    }
                return;
                }
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::ShowReplaceDialog()
        {
        auto* currentScript = GetCurrentEditor();
        if (currentScript != nullptr && m_findData.GetFindString().empty())
            {
            m_findData.SetFindString(currentScript->GetSelectedText());
            }

        if (m_dlgFind != nullptr)
            {
            m_dlgFind->Destroy();
            m_dlgFind = nullptr;
            }
        if (m_dlgReplace == nullptr)
            {
            m_dlgReplace =
                new wxFindReplaceDialog(this, &m_findData, _(L"Replace"), wxFR_REPLACEDIALOG);
            }
        m_dlgReplace->Show(true);
        m_dlgReplace->SetFocus();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::ShowFindDialog()
        {
        auto* currentScript = GetCurrentEditor();
        if (currentScript != nullptr && m_findData.GetFindString().empty())
            {
            m_findData.SetFindString(currentScript->GetSelectedText());
            }

        if (m_dlgReplace != nullptr)
            {
            m_dlgReplace->Destroy();
            m_dlgReplace = nullptr;
            }
        if (m_dlgFind == nullptr)
            {
            m_dlgFind = new wxFindReplaceDialog(this, &m_findData, _(L"Find"));
            }
        m_dlgFind->Show(true);
        m_dlgFind->SetFocus();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Undo()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->Undo();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Redo()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->Redo();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::DuplicateLine()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->LineDuplicate();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::GotoLineDialog()
        {
        auto* editor = GetCurrentEditor();
        if (editor == nullptr)
            {
            return;
            }

        const long lineCount = editor->GetLineCount();
        const long currentLine = editor->GetCurrentLine() + 1;
        const long newLine = wxGetNumberFromUser(_(L"Enter line number:"), wxEmptyString,
                                                 _(L"Go To Line"), currentLine, 1, lineCount, this);
        if (newLine != -1)
            {
            editor->GotoLine(newLine - 1);
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Comment()
        {
        auto* editor = GetCurrentEditor();
        if (editor == nullptr)
            {
            return;
            }

        const int lineStart = editor->LineFromPosition(editor->GetSelectionStart());
        int lineEnd = editor->LineFromPosition(editor->GetSelectionEnd());

        // if the selection ends at the start of a line, don't include that line
        if (lineEnd > lineStart && editor->GetColumn(editor->GetSelectionEnd()) == 0)
            {
            lineEnd--;
            }

        editor->BeginUndoAction();
        for (int i = lineStart; i <= lineEnd; ++i)
            {
            editor->InsertText(editor->PositionFromLine(i), L"--");
            }
        editor->EndUndoAction();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Uncomment()
        {
        auto* editor = GetCurrentEditor();
        if (editor == nullptr)
            {
            return;
            }

        const int lineStart = editor->LineFromPosition(editor->GetSelectionStart());
        int lineEnd = editor->LineFromPosition(editor->GetSelectionEnd());

        if (lineEnd > lineStart && editor->GetColumn(editor->GetSelectionEnd()) == 0)
            {
            lineEnd--;
            }

        editor->BeginUndoAction();
        for (int i = lineStart; i <= lineEnd; ++i)
            {
            const wxString lineText = editor->GetLine(i);
            int firstCharPos = 0;
            while (std::cmp_less(firstCharPos, lineText.length()) &&
                   (lineText[firstCharPos] == L' ' || lineText[firstCharPos] == L'\t'))
                {
                firstCharPos++;
                }
            if (lineText.substr(firstCharPos).starts_with(L"--"))
                {
                editor->DeleteRange(editor->PositionFromLine(i) + firstCharPos, 2);
                }
            }
        editor->EndUndoAction();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Cut()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->Cut();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Copy()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->Copy();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::Paste()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->Paste();
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::SelectAll()
        {
        auto* editor = GetCurrentEditor();
        if (editor != nullptr)
            {
            editor->SelectAll();
            }
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::CanCut() const
        {
        const auto* editor = GetCurrentEditor();
        return editor != nullptr && editor->GetSelectionStart() != editor->GetSelectionEnd();
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::CanCopy() const
        {
        const auto* editor = GetCurrentEditor();
        return editor != nullptr && editor->GetSelectionStart() != editor->GetSelectionEnd();
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::CanPaste() const
        {
        const auto* editor = GetCurrentEditor();
        return editor != nullptr && editor->CanPaste();
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::CanUndo() const
        {
        const auto* editor = GetCurrentEditor();
        return editor != nullptr && editor->CanUndo();
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::CanRedo() const
        {
        const auto* editor = GetCurrentEditor();
        return editor != nullptr && editor->CanRedo();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::OnFindDialog(wxFindDialogEvent& event)
        {
        auto* currentScript = GetCurrentEditor();
        if (currentScript == nullptr)
            {
            return;
            }

        if (event.GetEventType() == wxEVT_FIND || event.GetEventType() == wxEVT_FIND_NEXT)
            {
            currentScript->OnFind(event);
            }
        else if (event.GetEventType() == wxEVT_FIND_REPLACE)
            {
            long from{ 0 }, to{ 0 };
            currentScript->GetSelection(&from, &to);
            currentScript->SetSelection(from, from);
            currentScript->SearchAnchor();

            auto foundPos = currentScript->FindNext(event.GetFindString(), event.GetFlags());
            if (foundPos != wxSTC_INVALID_POSITION)
                {
                if (from == foundPos &&
                    std::cmp_equal(to, foundPos + event.GetFindString().length()))
                    {
                    currentScript->Replace(foundPos, foundPos + event.GetFindString().length(),
                                           event.GetReplaceString());
                    currentScript->SetSelection(foundPos,
                                                foundPos + event.GetReplaceString().length());
                    currentScript->SearchAnchor();
                    foundPos = currentScript->FindNext(event.GetFindString(), event.GetFlags());
                    if (foundPos != wxSTC_INVALID_POSITION)
                        {
                        currentScript->SetSelection(foundPos,
                                                    foundPos + event.GetFindString().length());
                        }
                    }
                else
                    {
                    currentScript->SetSelection(foundPos,
                                                foundPos + event.GetFindString().length());
                    currentScript->SearchAnchor();
                    }
                }
            else
                {
                currentScript->SetSelection(from, to);
                currentScript->SearchAnchor();
                wxMessageBox(_(L"No further occurrences found."), _(L"Item Not Found"),
                             wxOK | wxICON_INFORMATION, this);
                }
            }
        else if (event.GetEventType() == wxEVT_FIND_REPLACE_ALL)
            {
            currentScript->SetSelection(0, 0);
            currentScript->SearchAnchor();
            auto foundPos = currentScript->FindNext(event.GetFindString(), event.GetFlags(), false);
            while (foundPos != wxSTC_INVALID_POSITION)
                {
                currentScript->Replace(foundPos, foundPos + event.GetFindString().length(),
                                       event.GetReplaceString());
                currentScript->SetSelection(foundPos + event.GetReplaceString().length(),
                                            foundPos + event.GetReplaceString().length());
                currentScript->SearchAnchor();
                foundPos = currentScript->FindNext(event.GetFindString(), event.GetFlags(), false);
                }
            }
        else if (event.GetEventType() == wxEVT_FIND_CLOSE)
            {
            if (m_dlgReplace != nullptr)
                {
                m_dlgReplace->Destroy();
                m_dlgReplace = nullptr;
                }
            if (m_dlgFind != nullptr)
                {
                m_dlgFind->Destroy();
                m_dlgFind = nullptr;
                }
            }
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::RefreshDebugWindow()
        {
        if (m_debugMessageWindow == nullptr)
            {
            return;
            }
        const auto debugReportBody =
            L"<!DOCTYPE html>\n<html>\n<head><meta name=\"color-scheme\" content=\"light dark\">"
            L"<style>html,body{height:100%;margin:0;background-color:Canvas;color:CanvasText;}"
            L"body{font-family:system-ui,sans-serif,'Apple Color Emoji','Segoe UI Emoji',"
            L"'Noto Color Emoji','Twemoji "
            L"Mozilla';font-size:11pt;padding:10px;box-sizing:border-box;}"
            L"</style></head>\n<body>" +
            m_debugContent + L"\n</body>\n</html>";
        m_debugMessageWindow->SetPage(debugReportBody, wxString{});
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::DebugOutput(const wxString& str)
        {
        m_debugContent += L"\n<br />" + str;
        RefreshDebugWindow();
        }

    //-------------------------------------------------------
    void ScriptWorkbenchPanel::DebugClear()
        {
        m_debugContent = wxString::Format(
            // TRANSLATORS: %d placeholders are the major, minor, and release version numbers of Lua
            _(L"Lua version %d.%d.%d, ready..."), LUA_VERSION_MAJOR_N, LUA_VERSION_MINOR_N,
            LUA_VERSION_RELEASE_N);
        RefreshDebugWindow();
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::PromptToSaveUnsavedScripts()
        {
        for (auto& entry : m_scripts)
            {
            auto* codeEditor = entry.m_editor;
            if (codeEditor == nullptr || !codeEditor->GetModify())
                {
                continue;
                }
            // show which script the prompt is about
            m_scriptSidebar->SelectSubItemById(m_scriptsFolderId, entry.m_sidebarId, false, false);
            const wxString name = codeEditor->GetScriptFilePath().empty() ?
                                      wxString{ _(L"(unnamed)") } :
                                      wxFileName{ codeEditor->GetScriptFilePath() }.GetName();
            const int answer =
                wxMessageBox(wxString::Format(_(L"Save changes to '%s'?"), name), _(L"Save Script"),
                             wxYES_NO | wxCANCEL | wxICON_QUESTION);
            if (answer == wxCANCEL)
                {
                return false;
                }
            if (answer != wxYES)
                {
                continue;
                }
            if (!codeEditor->Save())
                {
                return false;
                }
            const int idx = m_editorBook->FindPage(entry.m_page);
            if (idx != wxNOT_FOUND)
                {
                m_editorBook->SetPageText(idx,
                                          wxFileName{ codeEditor->GetScriptFilePath() }.GetName());
                }
            UpdateDirtyMark(codeEditor);
            }
        return true;
        }

    //-------------------------------------------------------
    bool ScriptWorkbenchPanel::HasUnsavedOrLoadedScripts() const
        {
        return std::ranges::any_of(m_scripts,
                                   [](const auto& entry)
                                   {
                                       return entry.m_editor != nullptr &&
                                              (entry.m_editor->GetModify() ||
                                               !entry.m_editor->GetScriptFilePath().empty());
                                   });
        }
    } // namespace Wisteria::UI
