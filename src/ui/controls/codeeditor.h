/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __CODE_EDITOR_H__
#define __CODE_EDITOR_H__

#include "../../util/donttranslate.h"
#include "../../util/string_util.h"
#include <map>
#include <set>
#include <vector>
#include <wx/fdrepdlg.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/validate.h>

/// @brief Namespace for code/formula parsing and editing.
namespace Wisteria::UI
    {
    /** @brief A @c wxStyledTextCtrl-derived editor designed for code editing.

        You can specify a code language via SetLanguage(), and the editor
        will handle loading that language's keywords, lexer, highlighting,
        class & library separators (for autocompletion), and file filter.

        This editor offers a simplified interface for loading a list of functions
        and libraries/classes (with sub-functions) that will then be recognized by
        autocompletion and highlighter.

        Also included is built-in file opening and saving support,
        as well as simplified searching functions.

        @par Example:
        @code
        // core Lua keywords will now be loaded for highlighting
        codeEditor = new CodeEditor(theParentDlg, wxSTC_LEX_LUA);

        // add a set of global math functions
        CodeEditor::NameList MathFunctions;
        MathFunctions.insert(wxString(L"SIN(x)\t" +
            _(L"Sine of x, which returns the sine of the angle X in radians.")).ToStdWstring());
        MathFunctions.insert(wxString("COS(x)\t" +
            _(L"Cosine of x, returns the cosine of the angle X in radians.")).ToStdWstring());

        codeEditor->AddFunctionsOrClasses(MathFunctions);

        // or, add these functions inside of a library named "Math"
        codeEditor->AddLibrary(L"Math", MathFunctions);

        // merge all custom functions, libraries, and classes into the
        // autocompletion and highlighting systems
        codeEditor->Finalize();
        @endcode
    */
    class CodeEditor final : public wxStyledTextCtrl
        {
      public:
        /// @brief Container type for function, class, and category names.
        using NameList = std::set<std::wstring, string_util::string_no_case_less>;
        /** @brief Constructor.
            @param parent The parent window.
            @param lang Sets the language used in this editor.\n
                @c wxSTC_LEX_LUA, @wxSTC_LEX_HTML, @c wxSTC_LEX_CPP, and @c wxSTC_LEX_CPPNOCASE
                are currently supported.
            @param id The ID for this editor.
            @param pos The position.
            @param size The size of the editor.
            @param style The window style for this editor.
            @param name The class name for this window.*/
        explicit CodeEditor(wxWindow* parent, const int lang,
                            wxWindowID id = wxID_ANY,
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize, long style = 0,
                            const wxString& name = L"CodeEditor");
        /// @private
        CodeEditor(const CodeEditor&) = delete;
        /// @private
        CodeEditor& operator=(const CodeEditor&) = delete;

        /** @brief Adds a library and its functions/classes.
                This information is used for autocompletion.
            @param library The name of the library.
            @param[in,out] functions The classes and functions inside of the library.
                The syntax for these strings is
                the name of the function and (optionally) a return type following a tab character.
                For example, `"GetUser()->User"` will load a function named `GetUser`
                with a return type of `User`.
            @sa Finalize().*/
        void AddLibrary(const wxString& library, NameList& functions);
        /** @brief Adds a class and its functions. This information is used for autocompletion.
            @param theClass The name of the class.
            @param[in,out] functions The functions inside of the class.
                The syntax for these strings is the name of the function and
                (optionally) a return type following a tab character.
                For example, `"GetUser()->User"` will load a function
                named `GetUser` with a return type of `User`.
            @sa Finalize().*/
        void AddClass(const wxString& theClass, NameList& functions);
        /** @brief Adds a set of function or class names that the highlighting and
                auto-completion should recognize.
            @param functions The array of functions to add.
            @sa Finalize().*/
        void AddFunctionsOrClasses(const NameList& functions);
        /// Call this after adding all the functions/classes/libraries.
        /// @sa AddFunctionsOrClasses(), AddClass(), AddLibrary().
        void Finalize();

        /** @brief Sets whether to include the line-number margins.
            @param include Set to @c true to include the line-number margins,
                @c false to hide them.*/
        void IncludeNumberMargin(const bool include)
            {
            SetMarginWidth(0, include ? TextWidth(wxSTC_STYLE_LINENUMBER, L"_999999") : 0);
            }

        /** @brief Sets whether to include the code-folding margins.
            @param include Set to @c true to include the code-folding margins,
                @c false to hide them.*/
        void IncludeFoldingMargin(const bool include) { SetMarginWidth(1, include ? 16 : 0); }

        /// @returns The filepath where the script is currently being saved to.
        [[nodiscard]]
        const wxString& GetScriptFilePath() const noexcept
            {
            return m_scriptFilePath;
            }

        /** @brief Sets the path of where the script is being saved.
            @param filePath The filepath of the script.*/
        void SetScriptFilePath(const wxString& filePath) { m_scriptFilePath = filePath; }

        /// @brief Saves the script.
        /// @note If the script's filepath has not been set, then will prompt for a path.
        /// @returns @c true if file was saved.
        /// @sa SetScriptFilePath().
        bool Save();
        /// @brief Prompts for a Lua script and opens it.
        /// @returns @c true if a script was opened, false if opening was cancelled.
        bool Open();
        /// @brief Closes the currently open script file and creates a blank one.
        void New();
        /** @brief Search forwards (from the cursor) for a string and moves the
                selection to it (if found).
            @param textToFind The text to find.
            @param searchFlags How to search. Can be a combination of
                @c wxSTC_FIND_WHOLEWORD, @c wxSTC_FIND_MATCHCASE, @c wxSTC_FIND_WORDSTART,
                @c wxSTC_FIND_REGEXP, and @c wxSTC_FIND_POSIX.
            @param wrapAround @c true to search from the top once the
                end of the document is reached.
            @returns The position of the match, or @c wxSTC_INVALID_POSITION if not found*/
        long FindNext(const wxString& textToFind, const int searchFlags = 0,
                      const bool wrapAround = true);
        /** @brief Search backwards (from the cursor) for a string and moves the
                selection to it (if found).
            @param textToFind The text to find.
            @param searchFlags How to search. Can be a combination of
                @c wxSTC_FIND_WHOLEWORD, @c wxSTC_FIND_MATCHCASE, @c wxSTC_FIND_WORDSTART,
                @c wxSTC_FIND_REGEXP, and @c wxSTC_FIND_POSIX.
            @returns The position of the match, or @c wxSTC_INVALID_POSITION if not found.*/
        long FindPrevious(const wxString& textToFind, const int searchFlags = 0);

        /** @brief When creating a new script, this will be the first line always included.
                This is useful if there is another Lua script always included in new scripts.
                An example of this could be `SetDefaultHeader(L"dofile(\"AppLibrary.lua\")")`.
            @param header The string to include at the top of all new scripts.*/
        void SetDefaultHeader(const wxString& header) { m_defaultHeader = header; }

        /// @returns The default header being included in all new scripts.
        /// @sa SetDefaultHeader().
        [[nodiscard]]
        const wxString& GetDefaultHeader() const noexcept
            {
            return m_defaultHeader;
            }

        /** @brief For autocompletion, this sets the character that divides a library/namespace
                from its member classes/functions.
            @param ch The separator character.*/
        void SetLibraryAccessor(const wchar_t ch) noexcept { m_libraryAccessor = ch; }

        /// @returns The separator between libraries/namespaces and their member classes/functions.
        [[nodiscard]]
        wchar_t GetLibraryAccessor() const noexcept
            {
            return m_libraryAccessor;
            }

        /** @brief For autocompletion, this sets the character that divides an object from
                its member functions.
            @param ch The separator character.*/
        void SetObjectAccessor(const wchar_t ch) noexcept { m_objectAccessor = ch; }

        /// @returns The separator between objects and their member functions.
        [[nodiscard]]
        wchar_t GetObjectAccessor() const noexcept
            {
            return m_objectAccessor;
            }

        /** @brief Sets the file filter for the Open dialog.
            @param filter The file filter.*/
        void SetFileFilter(const wxString& filter) { m_fileFilter = filter; }

        /// @returns The file filter used when opening a script.
        [[nodiscard]]
        const wxString& GetFileFilter() const noexcept
            {
            return m_fileFilter;
            }


        /// @brief Can be useful when calling find event from parent container.
        /// @private
        void OnFind(wxFindDialogEvent& event);

        /// @brief The style to use for error annotations.
        constexpr static int ERROR_ANNOTATION_STYLE = wxSTC_STYLE_LASTPREDEFINED + 1;

      private:
        struct wxStringCmpNoCase
            {
            [[nodiscard]]
            bool operator()(const wxString& s1, const wxString& s2) const
                {
                return s1.CmpNoCase(s2) < 0;
                }
            };

        struct wxStringPartialCmpNoCase
            {
            [[nodiscard]]
            bool operator()(const wxString& s1, const wxString& s2) const
                {
                return s1.CmpNoCase(s2.substr(0, s1.length())) < 0;
                }
            };

        /// @brief Sets the main color for the control. This will be the background color,
        ///     and all text colors will be adjusted to contrast against this color.
        /// @param background The color for the control's background.
        void SetThemeColor(const wxColour& background);
        void SetLanguage(const int lang);
        static bool SplitFunctionAndParams(wxString& function, wxString& params);
        [[nodiscard]]
        static wxString StripExtraInfo(const wxString& function);
        [[nodiscard]]
        static wxString StripReturnType(const wxString& function);
        [[nodiscard]]
        static wxString GetReturnType(const wxString& function);

        void OnMarginClick(wxStyledTextEvent& event);
        void OnCharAdded(wxStyledTextEvent& event);
        void OnAutoCompletionSelected(wxStyledTextEvent& event);
        void OnKeyDown(wxKeyEvent& event);

        void ResetActiveFunctionMap() noexcept { m_activeFunctionsAndSignaturesMap = nullptr; }

        using wxStringNoCaseMap = std::map<wxString, wxString, wxStringCmpNoCase>;
        // Library name, string of function names, and map of function names
        // and their respective full signatures
        using ObjectAndFunctionsMap =
            std::map<wxString, std::pair<wxString, wxStringNoCaseMap>, wxStringCmpNoCase>;

        ObjectAndFunctionsMap m_libraryCollection;
        ObjectAndFunctionsMap m_classCollection;

        wxStringNoCaseMap m_libraryFunctionsWithReturnTypes;
        std::set<wxString, wxStringPartialCmpNoCase> m_libaryAndClassNames;
        wxString m_libaryAndClassNamesStr;

        const wxStringNoCaseMap* m_activeFunctionsAndSignaturesMap{ nullptr };

        int m_lexer{ wxSTC_LEX_LUA };

        wxString m_scriptFilePath;

        wxString m_defaultHeader;

        wxString m_fileFilter;

        wchar_t m_libraryAccessor{ L'.' };
        wchar_t m_objectAccessor{ L':' };

        wxColour m_commentColor{ L"#008000" };
        wxColour m_commentDocColor{ L"#BEB7E7" };
        wxColour m_keywordColor1{ L"#559CD6" };
        wxColour m_keywordColor2{ L"#4EC9B0" };
        wxColour m_operatorColor{ L"#909EA9" };
        wxColour m_stringColor{ L"#D69D85" };

        wxDECLARE_CLASS(CodeEditor);
        };
    } // namespace Wisteria::UI

/** @}*/

#endif //__CODE_EDITOR_H__
