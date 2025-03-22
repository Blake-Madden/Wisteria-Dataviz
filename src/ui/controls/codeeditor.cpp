/////////////////////////////////////////////////////////////////////////////
// Name:        codeeditor.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "codeeditor.h"
#include "../../base/colorbrewer.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>

using namespace Wisteria::UI;
using namespace Wisteria::Colors;

wxIMPLEMENT_CLASS(CodeEditor, wxStyledTextCtrl)

    //-------------------------------------------------------------
    CodeEditor::CodeEditor(wxWindow* parent, const int lang, wxWindowID id /*=wxID_ANY*/,
                           const wxPoint& pos /*=wxDefaultPosition*/,
                           const wxSize& size /*=wxDefaultSize*/, long style /*=0*/,
                           const wxString& name /*"CodeEditor"*/)
    : wxStyledTextCtrl(parent, id, pos, size, style, name)
    {
    StyleClearAll();
    const wxFont font{ wxFontInfo().Family(wxFONTFAMILY_MODERN) };
    for (auto i = 0; i < wxSTC_STYLE_LASTPREDEFINED; ++i)
        {
        StyleSetFont(i, font.Larger().Larger().Bold());
        }

    // code-folding options
    SetProperty(L"fold", L"1");
    SetProperty(L"fold.compact", L"1");
    const wxColour markerColor{ wxSystemSettings::SelectLightDark(*wxWHITE, *wxBLACK) };
    const wxColour markerContrastColor{ wxSystemSettings::SelectLightDark(*wxBLACK, *wxWHITE) };
    MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_DOTDOTDOT, markerColor, markerColor);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_ARROWDOWN, markerColor, markerColor);
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY, markerColor, markerColor);
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_DOTDOTDOT, markerColor, markerContrastColor);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_ARROWDOWN, markerColor,
                 markerContrastColor);
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY, markerColor, markerColor);
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY, markerColor, markerColor);
    // margin settings
    SetMarginType(0, wxSTC_MARGIN_NUMBER);
    SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    SetMarginMask(1, wxSTC_MASK_FOLDERS);
    SetMarginWidth(1, FromDIP(16));
    SetMarginSensitive(1, true);
    SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
    // turn off tabs
    SetUseTabs(false);
    SetTabWidth(4);
    // enable auto-completion
    AutoCompSetIgnoreCase(true);
    AutoCompSetAutoHide(true);
    AutoCompSetMaxHeight(30);
    // annotations styles
    StyleSetBackground(
        ERROR_ANNOTATION_STYLE,
        wxSystemSettings::SelectLightDark(wxColour(244, 220, 220), wxColour(100, 100, 100)));
    StyleSetSizeFractional(ERROR_ANNOTATION_STYLE,
                           (StyleGetSizeFractional(wxSTC_STYLE_DEFAULT) * 4) / 5);
    // turn on annotations
    AnnotationSetVisible(wxSTC_ANNOTATION_BOXED);

    CallTipUseStyle(40);

    Bind(wxEVT_KEY_DOWN, &CodeEditor::OnKeyDown, this);
    Bind(wxEVT_FIND, &CodeEditor::OnFind, this, wxID_ANY);
    Bind(wxEVT_FIND_NEXT, &CodeEditor::OnFind, this, wxID_ANY);
    Bind(wxEVT_STC_MARGINCLICK, &CodeEditor::OnMarginClick, this, wxID_ANY);
    Bind(wxEVT_STC_CHARADDED, &CodeEditor::OnCharAdded, this, wxID_ANY);
    Bind(wxEVT_STC_AUTOCOMP_SELECTION, &CodeEditor::OnAutoCompletionSelected, this, wxID_ANY);

    SetLanguage(lang);

    SetThemeColor(wxSystemSettings::GetColour(wxSystemColour::wxSYS_COLOUR_WINDOW));
    }

//-------------------------------------------------------------
void CodeEditor::SetThemeColor(const wxColour& background)
    {
    const wxColour foreground = ColorContrast::BlackOrWhiteContrast(background);
    ColorContrast contrast(background);

    StyleSetBackground(wxSTC_STYLE_DEFAULT, background);
    StyleSetForeground(wxSTC_STYLE_DEFAULT, foreground);

    for (int i = wxSTC_LUA_DEFAULT; i <= wxSTC_LUA_LABEL; ++i)
        {
        StyleSetBackground(i, background);
        StyleSetForeground(i, foreground);
        }

    if (wxSTC_LEX_LUA == m_lexer)
        {
        StyleSetForeground(wxSTC_LUA_WORD, contrast.Contrast(m_keywordColor1));
        StyleSetForeground(wxSTC_LUA_WORD2, contrast.Contrast(m_keywordColor2));
        StyleSetForeground(wxSTC_LUA_STRING, contrast.Contrast(m_stringColor));
        StyleSetForeground(wxSTC_LUA_OPERATOR, contrast.Contrast(m_operatorColor));
        StyleSetForeground(wxSTC_LUA_COMMENTLINE, contrast.Contrast(m_commentColor));
        }
    else if (wxSTC_LEX_CPP == m_lexer || wxSTC_LEX_CPPNOCASE == m_lexer)
        {
        StyleSetForeground(wxSTC_C_WORD, contrast.Contrast(m_keywordColor1));
        StyleSetForeground(wxSTC_C_WORD2, contrast.Contrast(m_keywordColor2));
        StyleSetForeground(wxSTC_C_STRING, contrast.Contrast(m_stringColor));
        StyleSetForeground(wxSTC_C_OPERATOR, contrast.Contrast(m_operatorColor));
        StyleSetForeground(wxSTC_C_COMMENTLINE, contrast.Contrast(m_commentColor));
        StyleSetForeground(wxSTC_C_COMMENT, contrast.Contrast(m_commentColor));
        StyleSetForeground(wxSTC_C_COMMENTLINEDOC, contrast.Contrast(m_commentColor));
        StyleSetForeground(wxSTC_C_COMMENTDOC, contrast.Contrast(m_commentColor));
        StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, contrast.Contrast(m_commentDocColor));
        StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, contrast.Contrast(m_commentDocColor));
        }
    else if (wxSTC_LEX_HTML == m_lexer)
        {
        StyleSetForeground(wxSTC_H_ENTITY, contrast.Contrast(m_keywordColor1));
        StyleSetForeground(wxSTC_H_TAG, contrast.Contrast(m_keywordColor1));
        StyleSetForeground(wxSTC_H_TAGUNKNOWN, contrast.Contrast(m_keywordColor1));
        StyleSetForeground(wxSTC_H_ATTRIBUTE, contrast.Contrast(m_keywordColor2));
        StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, contrast.Contrast(m_keywordColor2));
        StyleSetForeground(wxSTC_H_DOUBLESTRING, contrast.Contrast(m_stringColor));
        StyleSetForeground(wxSTC_H_SINGLESTRING, contrast.Contrast(m_stringColor));
        StyleSetForeground(wxSTC_H_COMMENT, contrast.Contrast(m_commentColor));
        StyleSetForeground(wxSTC_H_XCCOMMENT, contrast.Contrast(m_commentColor));
        }

    MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_DOTDOTDOT, foreground, background);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_ARROWDOWN, foreground, background);
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY, foreground, background);
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_DOTDOTDOT, foreground, background);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_ARROWDOWN, foreground, background);
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY, foreground, background);
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY, foreground, background);

    SetCaretForeground(foreground);
    }

//-------------------------------------------------------------
void CodeEditor::SetLanguage(const int lang)
    {
    m_lexer = lang;
    SetLexer(m_lexer);
    if (wxSTC_LEX_LUA == m_lexer)
        {
        // core language keywords
        SetKeyWords(0, _DT(L"and break do else elseif end false for function if in local "
                           "nil not or repeat return then true until while dofile"));
        // other language settings
        SetFileFilter(_(L"Lua Scripts (*.lua)|*.lua"));
        SetLibraryAccessor(L'.');
        SetObjectAccessor(L':');
        }
    else if (wxSTC_LEX_CPP == m_lexer || wxSTC_LEX_CPPNOCASE == m_lexer)
        {
        // core language keywords
        SetKeyWords(
            0, _DT(L"alignas alignof and_eq asm atomic_cancel atomic_commit atomic_noexcept auto "
                   "bitand bitor bool break case catch char char8_t char16_t char32_t class compl "
                   "concept const consteval constexpr constinit const_cast continue co_await "
                   "co_return co_yield decltype default delete do double dynamic_cast else enum "
                   "explicit export extern false float for friend goto if inline int long mutable "
                   "namespace new noexcept not not_eq nullptr operator or or_eq private protected "
                   "public reflexpr register reinterpret_cast requires return short signed "
                   "sizeof static static_assert static_cast struct switch synchronized template "
                   "this thread_local throw true try typedef typeid typename "
                   "union unsigned using virtual void volatile wchar_t while xor xor_eq "
                   "final override import module"));
        // other language settings
        SetFileFilter(_(L"C++ Source Files (*.cpp;*.c;*.h;*.hpp)|*.cpp;*.c;*.h;*.hpp"));
        SetLibraryAccessor(L':');
        SetObjectAccessor(L'.');
        }
    else if (wxSTC_LEX_HTML == m_lexer)
        {
        SetFileFilter(_(L"HTML Files (*.html;*.htm)|*.html;*.htm"));
        }
    }

//-------------------------------------------------------------
void CodeEditor::New()
    {
    if (GetModify())
        {
        if (wxMessageBox(_(L"Do you wish to save your unsaved changes?"), _(L"Save Script"),
                         wxYES_NO | wxICON_QUESTION, this) == wxYES)
            {
            Save();
            }
        }
    SetText(m_defaultHeader);
    SetSelection(GetLastPosition(), GetLastPosition());
    SetModified(false);
    SetFocus();

    SetScriptFilePath(wxString{});
    }

//-------------------------------------------------------------
bool CodeEditor::Open()
    {
    if (GetModify())
        {
        if (wxMessageBox(_(L"Do you wish to save your unsaved changes?"), _(L"Save Script"),
                         wxYES_NO | wxICON_QUESTION, this) == wxYES)
            {
            Save();
            }
        }
    wxFileDialog dialogOpen(this, _(L"Select Script to Open"), wxString{}, wxString{},
                            GetFileFilter(), wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);
    if (dialogOpen.ShowModal() != wxID_OK)
        {
        return false;
        }
    const wxString filePath = dialogOpen.GetPath();

    wxWindowUpdateLocker noUpdates(this);
    ClearAll();
    LoadFile(filePath);
    SetSelection(0, 0);
    SetScriptFilePath(filePath);

    return true;
    }

//-------------------------------------------------------------
bool CodeEditor::Save()
    {
    if (GetScriptFilePath().empty())
        {
        wxFileDialog dialogSave(this, _(L"Save Script As"), wxString{}, wxString{}, GetFileFilter(),
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dialogSave.ShowModal() != wxID_OK)
            {
            return false;
            }
        SetScriptFilePath(dialogSave.GetPath());
        }
    if (!SaveFile(GetScriptFilePath()))
        {
        wxMessageBox(wxString::Format(_(L"Unable to save file \"%s\"."), GetScriptFilePath()),
                     _(L"Error"), wxOK | wxICON_EXCLAMATION, this);
        return false;
        }
    return true;
    }

//-------------------------------------------------------------
void CodeEditor::OnKeyDown(wxKeyEvent& event)
    {
    if (event.ControlDown() && event.GetKeyCode() == L'S')
        {
        Save();
        }
    else if (event.ControlDown() && event.GetKeyCode() == L'O')
        {
        Open();
        }
    else
        {
        event.Skip();
        }
    }

//-------------------------------------------------------------
void CodeEditor::OnFind(wxFindDialogEvent& event)
    {
    const int flags = event.GetFlags();
    int searchFlags = 0;
    if (flags & wxFR_MATCHCASE)
        {
        searchFlags = searchFlags | wxSTC_FIND_MATCHCASE;
        }
    if (flags & wxFR_WHOLEWORD)
        {
        searchFlags = searchFlags | wxSTC_FIND_WHOLEWORD;
        }

    long foundPos{ wxSTC_INVALID_POSITION };
    if (flags & wxFR_DOWN)
        {
        foundPos = FindNext(event.GetFindString(), searchFlags);
        }
    else
        {
        foundPos = FindPrevious(event.GetFindString(), searchFlags);
        }

    if (foundPos == wxSTC_INVALID_POSITION)
        {
        wxMessageBox(_(L"No further occurrences found."), _(L"Item Not Found"),
                     wxOK | wxICON_INFORMATION, this);
        }
    }

//-------------------------------------------------------------
long CodeEditor::FindPrevious(const wxString& textToFind, const int searchFlags /*= 0*/)
    {
    SearchAnchor();
    long selStart{ 0 }, selEnd{ 0 };
    GetSelection(&selStart, &selEnd);
    auto foundPos = SearchPrev(searchFlags, textToFind);
    if (foundPos != wxSTC_INVALID_POSITION)
        {
        SetSelection(foundPos, foundPos + textToFind.length());
        SearchAnchor();
        EnsureCaretVisible();
        }
    return foundPos;
    }

//-------------------------------------------------------------
long CodeEditor::FindNext(const wxString& textToFind, const int searchFlags /*= 0*/,
                          const bool wrapAround /*= true*/)
    {
    SearchAnchor();
    long selStart, selEnd;
    GetSelection(&selStart, &selEnd);
    SetSelection(selEnd, selEnd);
    SearchAnchor();
    auto foundPos = SearchNext(searchFlags, textToFind);
    if (foundPos != wxSTC_INVALID_POSITION)
        {
        SetSelection(foundPos, foundPos + textToFind.length());
        SearchAnchor();
        EnsureCaretVisible();
        return foundPos;
        }
    // not found going forward, so start from beginning and try from there
    else if (wrapAround)
        {
        foundPos = FindText(0, GetLength(), textToFind, searchFlags);
        if (foundPos != wxSTC_INVALID_POSITION)
            {
            SetSelection(foundPos, foundPos + textToFind.length());
            SearchAnchor();
            EnsureCaretVisible();
            return foundPos;
            }
        }
    SetSelection(selStart, selEnd);
    SearchAnchor();
    return foundPos;
    }

//-------------------------------------------------------------
void CodeEditor::AddFunctionsOrClasses(const NameList& functions)
    {
    ResetActiveFunctionMap();

    for (const auto& func : functions)
        {
        m_libaryAndClassNames.insert(StripExtraInfo(func.c_str()));
        }
    }

//-------------------------------------------------------------
void CodeEditor::AddLibrary(const wxString& library, NameList& functions)
    {
    ResetActiveFunctionMap();

    wxString functionString;
    wxStringNoCaseMap funcNamesAndSignatures;
    for (const auto& func : functions)
        {
        const wxString funcName = StripExtraInfo(func);
        const wxString returnTypeStr = GetReturnType(func);
        if (returnTypeStr.length())
            {
            m_libraryFunctionsWithReturnTypes.insert(
                std::make_pair(library + L"." + StripExtraInfo(func), returnTypeStr));
            }
        funcNamesAndSignatures.emplace(funcName, StripReturnType(func));
        functionString += L" " + funcName;
        }
    m_libraryCollection.insert(
        std::make_pair(library, std::make_pair(functionString, funcNamesAndSignatures)));
    m_libaryAndClassNames.insert(library);
    }

//-------------------------------------------------------------
void CodeEditor::AddClass(const wxString& theClass, NameList& functions)
    {
    ResetActiveFunctionMap();

    wxString functionString;
    wxStringNoCaseMap funcNamesAndSignatures;
    for (const auto& func : functions)
        {
        const wxString funcName = StripExtraInfo(func);
        funcNamesAndSignatures.emplace(funcName, StripReturnType(func));
        functionString += L" " + funcName;
        }
    m_classCollection.insert(
        std::make_pair(theClass, std::make_pair(functionString, funcNamesAndSignatures)));
    m_libaryAndClassNames.insert(theClass);
    }

//-------------------------------------------------------------
void CodeEditor::Finalize()
    {
    m_libaryAndClassNamesStr.clear();
    for (const auto& className : m_libaryAndClassNames)
        {
        m_libaryAndClassNamesStr += L" " + className;
        }
    if (wxSTC_LEX_CPPNOCASE == m_lexer)
        {
        SetKeyWords(1, m_libaryAndClassNamesStr.Lower());
        }
    else
        {
        SetKeyWords(1, m_libaryAndClassNamesStr);
        }
    }

//-------------------------------------------------------------
wxString CodeEditor::StripExtraInfo(const wxString& function)
    {
    const auto extraInfoStart = function.find_first_of(L"\t (");
    if (extraInfoStart != wxString::npos)
        {
        return function.substr(0, extraInfoStart);
        }
    else
        {
        return function;
        }
    }

//-------------------------------------------------------------
wxString CodeEditor::StripReturnType(const wxString& function)
    {
    const auto retSepStart = function.find(L"->");
    if (retSepStart != wxString::npos)
        {
        return function.substr(0, retSepStart);
        }
    else
        {
        return function;
        }
    }

//-------------------------------------------------------------
wxString CodeEditor::GetReturnType(const wxString& function)
    {
    const auto retSepStart = function.find(L"->");
    if (retSepStart != wxString::npos)
        {
        wxString returnType = function.substr(retSepStart + 2);
        returnType.Trim(true);
        returnType.Trim(false);
        return returnType;
        }
    else
        {
        return wxString{};
        }
    }

//-------------------------------------------------------------
bool CodeEditor::SplitFunctionAndParams(wxString& function, wxString& params)
    {
    const auto parenthesisStart = function.find(L'(');
    if (parenthesisStart != wxString::npos)
        {
        const auto parenthesisEnd = function.rfind(L')');
        // if empty parameter list then don't bother splitting this up
        if (parenthesisEnd == parenthesisStart + 1)
            {
            return false;
            }
        params = function.substr(parenthesisStart + 1, (parenthesisEnd - 1) - parenthesisStart);
        function.Truncate(parenthesisStart);
        return true;
        }
    return false;
    }

//-------------------------------------------------------------
void CodeEditor::OnMarginClick(wxStyledTextEvent& event)
    {
    if (event.GetMargin() == 1)
        {
        const int lineClick = LineFromPosition(event.GetPosition());
        if ((GetFoldLevel(lineClick) & wxSTC_FOLDLEVELHEADERFLAG) > 0)
            {
            ToggleFold(lineClick);
            }
        }
    }

//-------------------------------------------------------------
void CodeEditor::OnCharAdded(wxStyledTextEvent& event)
    {
    // Takes a word and a list of words and returns what from the list the word matched
    const auto matchFromList = [](const wxString& theWord, const auto& theList)
    {
        wxStringTokenizer tkz{ theList };
        wxString matchedFuncs;
        bool funcStartsWith{ false };
        while (tkz.HasMoreTokens())
            {
            const wxString nextToken = tkz.GetNextToken();
            if (nextToken.Lower().find(theWord.Lower()) != wxString::npos)
                {
                matchedFuncs.append(L' ').append(nextToken);
                // set closest match if we can
                if (!funcStartsWith && nextToken.Lower().starts_with(theWord))
                    {
                    funcStartsWith = true;
                    }
                }
            }
        matchedFuncs.Trim(true).Trim(false);
        return std::make_pair(funcStartsWith, matchedFuncs);
    };

    // Given a variable name, searches for the datatype of that variable based
    // on the first place it was assigned a value.
    const auto findDataType = [this](const int wordStart, const wxString objectName)
    {
        if (objectName.empty())
            {
            return wxString{};
            }

        int foundPos{ 0 };
        while (foundPos + objectName.length() + 2 < static_cast<size_t>(wordStart))
            {
            foundPos = FindText(foundPos, wordStart, objectName,
                                wxSTC_FIND_WHOLEWORD | wxSTC_FIND_MATCHCASE);
            if (foundPos != wxSTC_INVALID_POSITION &&
                foundPos + objectName.length() + 2 < static_cast<size_t>(wordStart))
                {
                foundPos += objectName.length();
                while (foundPos < GetLength() && GetCharAt(foundPos) == L' ')
                    {
                    ++foundPos;
                    }
                // found an assignment to this variable
                if (foundPos < GetLength() && GetCharAt(foundPos) == L'=')
                    {
                    // scan to whatever it is assigned to
                    do
                        {
                        ++foundPos;
                        } while (foundPos < GetLength() && GetCharAt(foundPos) == L' ');
                    return GetTextRange(foundPos, WordEndPosition(foundPos, true));
                    }
                else
                    {
                    continue;
                    }
                }
            else
                {
                break;
                }
            }
        return wxString{};
    };

    if (event.GetKey() == GetLibraryAccessor())
        {
        const int wordStart = WordStartPosition(GetCurrentPos() - 1, true);
        const wxString lastWord = GetTextRange(wordStart, GetCurrentPos() - 1);

        auto pos = m_libraryCollection.find(lastWord);
        if (pos != m_libraryCollection.cend())
            {
            m_activeFunctionsAndSignaturesMap = &pos->second.second;
            // show the full list of functions in the library
            AutoCompShow(0, pos->second.first);
            }
        }
    else if (event.GetKey() == L')')
        {
        CallTipCancel();
        }
    else if (event.GetKey() == L'(')
        {
        CallTipCancel();

        const int wordStart = WordStartPosition(GetCurrentPos() - 1, true);
        const wxString functionName = GetTextRange(wordStart, GetCurrentPos() - 1);

        // if a library function, then show its param list in a tooltip
        if (wordStart > 0 && GetCharAt(wordStart - 1) == GetLibraryAccessor())
            {
            const int libNameStart = WordStartPosition(wordStart - 1, true);
            const wxString libName = GetTextRange(libNameStart, wordStart - 1);

            auto libPos = m_libraryCollection.find(libName);
            if (libPos != m_libraryCollection.cend())
                {
                m_activeFunctionsAndSignaturesMap = &libPos->second.second;
                const auto foundFunc = libPos->second.second.find(functionName);
                if (foundFunc != libPos->second.second.cend())
                    {
                    wxString params, foundKeyword{ foundFunc->second };
                    if (SplitFunctionAndParams(foundKeyword, params))
                        {
                        CallTipShow(GetCurrentPos(), L"(" + params + L")");
                        }
                    }
                }
            }
        // or an object
        else if (wordStart > 0 && GetCharAt(wordStart - 1) == GetObjectAccessor())
            {
            int objectNameStart = WordStartPosition(wordStart - 1, false);
            const wxString objectName = GetTextRange(objectNameStart, wordStart - 1);

            if (objectName == L"()")
                {
                // step over function...
                int libNameEnd = WordStartPosition(objectNameStart - 1, false);
                // ...then step back to name of library
                objectNameStart = WordStartPosition(libNameEnd - 1, false);
                const wxString libraryNameAndFuncSig = GetTextRange(objectNameStart, wordStart - 3);
                auto libraryPos = m_libraryFunctionsWithReturnTypes.find(libraryNameAndFuncSig);
                if (libraryPos != m_libraryFunctionsWithReturnTypes.cend())
                    {
                    auto classPos = m_classCollection.find(libraryPos->second);
                    if (classPos != m_classCollection.cend())
                        {
                        m_activeFunctionsAndSignaturesMap = &classPos->second.second;
                        const auto foundFunc = classPos->second.second.find(functionName);
                        if (foundFunc != classPos->second.second.cend())
                            {
                            wxString params, foundKeyword{ foundFunc->second };
                            if (SplitFunctionAndParams(foundKeyword, params))
                                {
                                CallTipShow(GetCurrentPos(), L"(" + params + L")");
                                }
                            }
                        }
                    }
                }
            else
                {
                const wxString dataType = findDataType(wordStart, objectName);
                if (!dataType.empty())
                    {
                    // if it is a known class of ours, then show the functions
                    // available for that class
                    const auto classPos = m_classCollection.find(dataType);
                    if (classPos != m_classCollection.cend())
                        {
                        m_activeFunctionsAndSignaturesMap = &classPos->second.second;
                        const auto foundFunc = classPos->second.second.find(functionName);
                        if (foundFunc != classPos->second.second.cend())
                            {
                            wxString params, foundKeyword{ foundFunc->second };
                            if (SplitFunctionAndParams(foundKeyword, params))
                                {
                                CallTipShow(GetCurrentPos(), L"(" + params + L")");
                                }
                            }
                        }
                    }
                }
            }
        }
    else if (event.GetKey() == GetObjectAccessor())
        {
        int wordStart = WordStartPosition(GetCurrentPos() - 1, false);
        const wxString lastWord = GetTextRange(wordStart, GetCurrentPos() - 1);

        if (lastWord == L"()")
            {
            // step over function...
            wordStart = WordStartPosition(wordStart - 1, false);
            // ...then step back to name of library
            wordStart = WordStartPosition(wordStart - 1, false);
            const wxString libraryName = GetTextRange(wordStart, GetCurrentPos() - 3);
            auto libraryPos = m_libraryFunctionsWithReturnTypes.find(libraryName);
            if (libraryPos != m_libraryFunctionsWithReturnTypes.cend())
                {
                auto classPos = m_classCollection.find(libraryPos->second);
                if (classPos != m_classCollection.cend())
                    {
                    m_activeFunctionsAndSignaturesMap = &classPos->second.second;
                    AutoCompShow(0, classPos->second.first);
                    }
                }
            }
        else
            {
            // might be a variable, look for where it was first assigned to something
            const wxString dataType = findDataType(wordStart, lastWord);
            // if it is a known class of ours, then show the functions
            // available for that class
            const auto classPos = m_classCollection.find(dataType);
            if (classPos != m_classCollection.cend())
                {
                m_activeFunctionsAndSignaturesMap = &classPos->second.second;
                AutoCompShow(0, classPos->second.first);
                }
            }
        }
    else
        {
        const int wordStart = WordStartPosition(GetCurrentPos(), true);
        const wxString lastWord = GetTextRange(wordStart, GetCurrentPos());

        if (lastWord.length())
            {
            // see if we are inside a library; if so show its list of functions
            if (wordStart > 2 && GetCharAt(wordStart - 1) == GetLibraryAccessor())
                {
                const wxString libraryName =
                    GetTextRange(WordStartPosition(wordStart - 2, true), wordStart - 1);
                auto pos = m_libraryCollection.find(libraryName);
                if (pos != m_libraryCollection.cend())
                    {
                    m_activeFunctionsAndSignaturesMap = &pos->second.second;
                    // widdle the list of functions down to the ones that
                    // contain the current word
                    const auto [funcStartsWith, matchedFuncs] =
                        matchFromList(lastWord, pos->second.first);
                    AutoCompCancel();
                    if (!matchedFuncs.empty())
                        {
                        AutoCompShow(funcStartsWith ? lastWord.length() : 0, matchedFuncs);
                        }
                    }
                }
            // if an object...
            else if (wordStart > 2 && GetCharAt(wordStart - 1) == GetObjectAccessor())
                {
                int previousWordStart = WordStartPosition(wordStart - 2, false);
                const wxString previousWord = GetTextRange(previousWordStart, wordStart - 1);

                // see if it is an object returned from a known function
                if (previousWord == L"()")
                    {
                    // step over function...
                    previousWordStart = WordStartPosition(previousWordStart - 1, false);
                    // ...then step back to name of object
                    previousWordStart = WordStartPosition(previousWordStart - 1, false);
                    const wxString libraryName = GetTextRange(previousWordStart, wordStart - 1);
                    auto libraryPos = m_libraryFunctionsWithReturnTypes.find(libraryName);
                    if (libraryPos != m_libraryFunctionsWithReturnTypes.cend())
                        {
                        auto classPos = m_classCollection.find(libraryPos->second);
                        if (classPos != m_classCollection.cend())
                            {
                            m_activeFunctionsAndSignaturesMap = &classPos->second.second;
                            if (AutoCompActive())
                                {
                                AutoCompSelect(lastWord);
                                }
                            else
                                {
                                AutoCompShow(lastWord.length(), classPos->second.first);
                                }
                            }
                        }
                    }
                else
                    {
                    const wxString dataType = findDataType(wordStart, previousWord);
                    // if it is a known class of ours, then show the functions
                    // available for that class
                    const auto classPos = m_classCollection.find(dataType);
                    if (classPos != m_classCollection.cend())
                        {
                        m_activeFunctionsAndSignaturesMap = &classPos->second.second;
                        // widdle the list of functions down to the ones that
                        // contain the current word
                        const auto [funcStartsWith, matchedFuncs] =
                            matchFromList(lastWord, classPos->second.first);
                        AutoCompCancel();
                        if (!matchedFuncs.empty())
                            {
                            AutoCompShow(funcStartsWith ? lastWord.length() : 0, matchedFuncs);
                            }
                        }
                    }
                }
            // otherwise, we are at the global level, so show list of
            // high-level classes and libraries
            else
                {
                auto pos = m_libaryAndClassNames.find(lastWord);
                wxString foundKeyword, params;
                if (pos != m_libaryAndClassNames.cend())
                    {
                    foundKeyword = *pos;
                    SplitFunctionAndParams(foundKeyword, params);
                    }
                // if found a full keyword, then just fix its case and let it auto-highlight
                if (pos != m_libaryAndClassNames.cend() &&
                    foundKeyword.length() == lastWord.length())
                    {
                    SetSelection(wordStart, wordStart + lastWord.length());
                    // tooltip the parameters (if applicable)
                    if (params.length())
                        {
                        foundKeyword += L"(";
                        ReplaceSelection(foundKeyword);
                        SetSelection(wordStart + foundKeyword.length(),
                                     wordStart + foundKeyword.length());
                        params += L")";
                        CallTipShow(GetCurrentPos(), params);
                        }
                    else
                        {
                        ReplaceSelection(foundKeyword);
                        SetSelection(wordStart + foundKeyword.length(),
                                     wordStart + foundKeyword.length());
                        }
                    AutoCompCancel();
                    }
                // or if a partial find, then show auto-completion
                else
                    {
                    const auto [funcStartsWith, matchedFuncs] =
                        matchFromList(lastWord, m_libaryAndClassNamesStr);
                    AutoCompCancel();
                    if (!matchedFuncs.empty())
                        {
                        AutoCompShow(funcStartsWith ? lastWord.length() : 0, matchedFuncs);
                        }
                    }
                }
            }
        else
            {
            AutoCompCancel();
            }
        }
    event.Skip();
    }

//-------------------------------------------------------------
void CodeEditor::OnAutoCompletionSelected(wxStyledTextEvent& event)
    {
    wxString selectedString = event.GetText();

    if (m_activeFunctionsAndSignaturesMap != nullptr)
        {
        const auto foundFunc = m_activeFunctionsAndSignaturesMap->find(selectedString);
        if (foundFunc != m_activeFunctionsAndSignaturesMap->cend())
            {
            // use the full function signature instead of just its name
            selectedString = foundFunc->second;
            }
        }
    wxString paramText;
    const bool hasParams = SplitFunctionAndParams(selectedString, paramText);

    const int wordStart = WordStartPosition(GetCurrentPos(), true);
    const int wordEnd = WordEndPosition(GetCurrentPos(), true);
    SetSelection(wordStart, wordEnd);
    if (hasParams)
        {
        selectedString += L"(";
        }
    ReplaceSelection(selectedString);
    SetSelection(wordStart + selectedString.length(), wordStart + selectedString.length());
    AutoCompCancel();
    if (hasParams)
        {
        paramText += L")";
        CallTipShow(GetCurrentPos(), paramText);
        }
    ResetActiveFunctionMap();
    }
