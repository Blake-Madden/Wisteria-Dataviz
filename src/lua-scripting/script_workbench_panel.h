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

#ifndef SCRIPT_WORKBENCH_PANEL_H
#define SCRIPT_WORKBENCH_PANEL_H

#include "../ui/controls/codeeditor.h"
#include "../ui/controls/sidebar.h"
#include "../ui/dialogs/functionbrowserdlg.h"
#include "lua_interface.h"
#include <set>
#include <vector>
#include <wx/fdrepdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/simplebook.h>
#include <wx/splitter.h>
#include <wx/treelist.h>
#include <wx/webview.h>

class wxStyledTextCtrlMiniMap;

namespace Wisteria::UI
    {
    /// @brief Script workbench: script sidebar, single-editor area, debug output,
    ///     and an optional function-browser column.
    class ScriptWorkbenchPanel final : public wxPanel
        {
      public:
        /// @brief Constructor.
        /// @param parent The parent window.
        /// @param id The window ID.
        /// @param apiFilePrefix The prefix of the application's API resource files
        ///     (e.g., @c "rs" for @c rs-classes.api), which fill the function browser and
        ///     autocompletion. Leave empty if the application doesn't have any.
        /// @param defaultScriptHeader The text that new scripts start with.
        /// @note The application's Lua runner must be set before calling this.
        explicit ScriptWorkbenchPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
                                      wxString apiFilePrefix = wxString{},
                                      wxString defaultScriptHeader = wxString{});
        /// @private
        ~ScriptWorkbenchPanel() override;
        ScriptWorkbenchPanel(const ScriptWorkbenchPanel&) = delete;
        ScriptWorkbenchPanel& operator=(const ScriptWorkbenchPanel&) = delete;

        /// @name Operations (called by the ribbon / accelerators)
        /// @{

        /// @brief Adds a new, empty script.
        void NewScript();
        /// @brief Opens a script file.
        /// @param path The path of the file to open.
        void OpenScriptFromFile(const wxString& path);
        /// @brief Saves the current script.
        void SaveCurrentScript();
        /// @brief Runs the current editor's selected text, or its whole script
        ///     if nothing is selected.
        void RunCurrentScript();
        /// @brief Resumes a script paused at a breakpoint.
        void ContinueScript();
        /// @brief Stops the running script.
        void StopScript();
        /// @brief Restarts the Lua interpreter, clearing all global variables.
        void RestartInterpreter();
        /// @brief Shows or hides the function browser.
        void ToggleFunctionBrowser();
        /// @brief Shows the Find dialog.
        void ShowFindDialog();
        /// @brief Shows the Replace dialog.
        void ShowReplaceDialog();
        /// @brief Undoes the last edit in the current editor.
        void Undo();
        /// @brief Redoes the last undone edit in the current editor.
        void Redo();
        /// @brief Duplicates the current editor's current line.
        void DuplicateLine();
        /// @brief Prompts for a line number and goes to it in the current editor.
        void GotoLineDialog();
        /// @brief Comments out the selected lines in the current editor.
        void Comment();
        /// @brief Uncomments the selected lines in the current editor.
        void Uncomment();
        /// @brief Cuts the current editor's selection to the clipboard.
        void Cut();
        /// @brief Copies the current editor's selection to the clipboard.
        void Copy();
        /// @brief Pastes the clipboard's text into the current editor.
        void Paste();
        /// @brief Selects all text in the current editor.
        void SelectAll();
        /// @returns @c true if the current editor has an edit to undo.
        [[nodiscard]]
        bool CanUndo() const;
        /// @returns @c true if the current editor has an edit to redo.
        [[nodiscard]]
        bool CanRedo() const;
        /// @returns @c true if the current editor has a selection to cut.
        [[nodiscard]]
        bool CanCut() const;
        /// @returns @c true if the current editor has a selection to copy.
        [[nodiscard]]
        bool CanCopy() const;
        /// @returns @c true if the current editor can accept a paste.
        [[nodiscard]]
        bool CanPaste() const;
        /// @}

        /// @brief Append a message to the debug output window.
        /// @param str The message, which can contain HTML markup.
        void DebugOutput(const wxString& str);
        /// @brief Clear the debug output window.
        void DebugClear();

        /// @brief Prompts the user to save any dirty script buffers (Yes/No/Cancel
        ///     per script).
        /// @returns @c true if the caller may proceed (no dirty buffers, or every
        ///     prompt was answered Yes or No); @c false if the user cancelled,
        ///     in which case the calling close/exit flow should be aborted.
        bool PromptToSaveUnsavedScripts();

        /// @returns @c true if any open script buffer has unsaved changes,
        ///     or if any script other than the default untitled buffer is loaded.
        [[nodiscard]]
        bool HasUnsavedOrLoadedScripts() const;

        /// @returns @c true while a script is executing.
        [[nodiscard]]
        bool IsScriptRunning() const noexcept
            {
            return m_isScriptRunning;
            }

        /// @returns @c true if the function browser is currently shown.
        [[nodiscard]]
        bool IsFunctionBrowserVisible() const noexcept;

        /// @returns @c true if the debug window is currently shown.
        [[nodiscard]]
        bool IsDebugWindowVisible() const noexcept;

        /// @brief Toggles the debug window visibility.
        void ToggleDebugWindow();

        /// @brief Reapplies the currently active report theme's accent color to the
        ///     workbench's sidebars (the script list and the function browser's category list).
        void UpdateTheme();

      private:
        struct ScriptEntry
            {
            wxWindowID m_sidebarId{ wxID_ANY };
            wxWindow* m_page{ nullptr };
            CodeEditor* m_editor{ nullptr };
            wxStyledTextCtrlMiniMap* m_miniMap{ nullptr };
            };

        // attached to a Locals-window row that represents an expandable table;
        // tracks the table's registry handle and whether its placeholder child
        // has already been swapped out for its real entries
        class LocalsTableItemData : public wxClientData
            {
          public:
            explicit LocalsTableItemData(const int tableRef) : m_tableRef(tableRef) {}

            int m_tableRef{ LUA_NOREF };
            bool m_populated{ false };
            };

        void ImportAPI();
        void CreateControls();
        void BindEvents();

        void OnFindDialog(wxFindDialogEvent& event);
        void OnSidebarClick(wxCommandEvent& event);
        void OnLocalsItemExpanding(wxTreeListEvent& event);

        /// @brief Rebuilds the Locals window from the currently paused frame's
        ///     local variables.
        void PopulateLocalsWindow();
        /// @brief Clears the Locals window (called on resume/stop).
        void ClearLocalsWindow();
        /// @brief Appends @p info as a child of @p parent, seeding an expandable
        ///     table with a placeholder child so its arrow renders before it's
        ///     ever expanded (its real children are fetched lazily, on demand,
        ///     in OnLocalsItemExpanding()).
        void AddLocalsTreeItem(wxTreeListItem parent, const LuaInterpreter::LuaVariableInfo& info);

        /// @brief Callback registered with LuaInterpreter::SetPauseStateChangedCallback().
        /// @param line The 1-based line execution paused at, or -1 when resumed/ended.
        void OnLuaPauseStateChanged(int line);

        /// @brief Re-renders the debug webview.
        void RefreshDebugWindow();

        /// @returns @p zeroBasedLines (CodeEditor line numbers) converted to Lua's 1-based lines.
        [[nodiscard]]
        static std::set<int> ToLuaLines(const std::vector<int>& zeroBasedLines);

        /// @brief Updates the sidebar label of the entry that owns @p editor to
        ///     reflect its current dirty state (appends " *" when modified).
        void UpdateDirtyMark(CodeEditor* editor);

        /// @returns The editor for the currently-selected script, or nullptr.
        [[nodiscard]]
        CodeEditor* GetCurrentEditor() const;

        [[nodiscard]]
        CodeEditor* CreateLuaScript(wxWindow* parent);

        /// @brief Adds a new script entry (sidebar item + simplebook page),
        ///     selects it, and returns the created editor.
        CodeEditor* AddScriptEntry(const wxString& label);

        // Layout / controls
        wxSplitterWindow* m_outerSplitter{ nullptr };   // top: editor area, bottom: debug
        wxSplitterWindow* m_sidebarSplitter{ nullptr }; // left: sidebar, right: editor+funcbrowser
        wxSplitterWindow* m_funcBrowserSplitter{
            nullptr
        }; // left: editor book, right: func browser

        SideBar* m_scriptSidebar{ nullptr };
        wxSimplebook* m_editorBook{ nullptr };
        // bottom pane: tabbed "Output" (debug webview) and "Locals" (paused-frame variables)
        wxNotebook* m_debugNotebook{ nullptr };
        wxWebView* m_debugMessageWindow{ nullptr };
        wxTreeListCtrl* m_localsWindow{ nullptr };
        constexpr static unsigned int LOCALS_VALUE_COLUMN{ 1 };
        constexpr static unsigned int LOCALS_TYPE_COLUMN{ 2 };
        wxString m_debugContent;
        FunctionBrowserCtrl* m_functionBrowser{ nullptr };

        wxWindowID m_scriptsFolderId{ wxNewId() };
        int m_nextSidebarSubId{ 10000 };
        bool m_isScriptRunning{ false };
        // the editor actually executing/paused; may differ from GetCurrentEditor()
        // if the user switches tabs mid-run
        CodeEditor* m_runningEditor{ nullptr };

        std::vector<ScriptEntry> m_scripts;

        // find/replace
        wxFindReplaceData m_findData{ wxFR_DOWN };
        wxFindReplaceDialog* m_dlgFind{ nullptr };
        wxFindReplaceDialog* m_dlgReplace{ nullptr };

        // API tables imported from .api files
        using LibnameAndEntries = std::pair<std::wstring, CodeEditor::NameList>;
        std::vector<LibnameAndEntries> m_classes;
        std::vector<LibnameAndEntries> m_libraries;
        std::vector<LibnameAndEntries> m_enums;

        wxString m_apiFilePrefix;
        wxString m_defaultScriptHeader;
        };
    } // namespace Wisteria::UI

#endif // SCRIPT_WORKBENCH_PANEL_H
