/////////////////////////////////////////////////////////////////////////////
// Name:        codeeditor.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "codeeditor.h"
#include <wx/tokenzr.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include "../../base/colorbrewer.h"

using namespace Wisteria::UI;
using namespace Wisteria::Colors;

wxIMPLEMENT_CLASS(CodeEditor, wxStyledTextCtrl)

//-------------------------------------------------------------
CodeEditor::CodeEditor(wxWindow* parent, wxWindowID id/*=wxID_ANY*/,
    const wxPoint& pos/*=wxDefaultPosition*/,
    const wxSize& size/*=wxDefaultSize*/, long style/*=0*/,
    const wxString& name/*"CodeEditor"*/) :
    wxStyledTextCtrl(parent, id, pos, size, style, name)
    {
    StyleClearAll();
    const wxFont font(wxFontInfo(10).Family(wxFONTFAMILY_MODERN));
    for (auto i = 0; i < wxSTC_STYLE_LASTPREDEFINED; ++i)
        { StyleSetFont(i, font); }

    // code-folding options
    SetProperty(L"fold", L"1");
    SetProperty(L"fold.compact", L"1");
    MarkerDefine(wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_DOTDOTDOT, *wxBLACK, *wxBLACK);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_ARROWDOWN, *wxBLACK, *wxBLACK);
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_EMPTY,     *wxBLACK, *wxBLACK);
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_DOTDOTDOT, *wxBLACK, *wxWHITE);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_ARROWDOWN, *wxBLACK, *wxWHITE);
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY,     *wxBLACK, *wxBLACK);
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_EMPTY,     *wxBLACK, *wxBLACK);
    // margin settings
    SetMarginType(0, wxSTC_MARGIN_NUMBER);
    SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    SetMarginMask(1, wxSTC_MASK_FOLDERS);
    SetMarginWidth(1, FromDIP(16));
    SetMarginSensitive(1, true);
    SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED|wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
    // enable auto-completion
    AutoCompSetIgnoreCase(true);
    AutoCompSetAutoHide(true);

    CallTipUseStyle(40);

    Bind(wxEVT_KEY_DOWN, &CodeEditor::OnKeyDown, this);
    Bind(wxEVT_FIND, &CodeEditor::OnFind, this, wxID_ANY);
    Bind(wxEVT_FIND_NEXT, &CodeEditor::OnFind, this, wxID_ANY);
    Bind(wxEVT_STC_MARGINCLICK, &CodeEditor::OnMarginClick, this, wxID_ANY);
    Bind(wxEVT_STC_CHARADDED, &CodeEditor::OnCharAdded, this, wxID_ANY);
    Bind(wxEVT_STC_AUTOCOMP_SELECTION, &CodeEditor::OnAutoCompletionSelected, this, wxID_ANY);
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

    StyleSetForeground(wxSTC_LUA_WORD, contrast.Contrast(m_keywordColor));
    StyleSetForeground(wxSTC_LUA_WORD2, contrast.Contrast(m_keywordColor));
    StyleSetForeground(wxSTC_LUA_STRING, contrast.Contrast(m_stringColor));
    StyleSetForeground(wxSTC_LUA_OPERATOR, contrast.Contrast(m_operatorColor));
    StyleSetForeground(wxSTC_LUA_COMMENTLINE, contrast.Contrast(m_commentColor));

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
    if (wxSTC_LEX_LUA == lang)
        {
        // core language keywords
        SetLexer(lang);
        SetKeyWords(0,
            _DT(L"and break do else elseif end false for function if in local "
                 "nil not or repeat return then true until while"));
        // other language settings
        SetFileFilter(_(L"Lua Script (*.lua)|*.lua"));
        SetLibraryAccessor(L'.');
        SetObjectAccessor(L':');
        }

    // highlighting for all supported languages
    StyleSetForeground(wxSTC_LUA_WORD, m_keywordColor);
    StyleSetForeground(wxSTC_LUA_WORD2, m_keywordColor);
    StyleSetForeground(wxSTC_LUA_STRING, m_stringColor);
    StyleSetForeground(wxSTC_LUA_OPERATOR, m_operatorColor);
    StyleSetForeground(wxSTC_LUA_COMMENTLINE, m_commentColor);

    StyleSetBold(wxSTC_LUA_WORD, true);
    StyleSetBold(wxSTC_LUA_WORD2, true);
    StyleSetBold(wxSTC_LUA_OPERATOR, true);
    }

//-------------------------------------------------------------
void CodeEditor::New()
    {
    if (GetModify())
        {
        if (wxMessageBox(_(L"Do you wish to save your unsaved changes?"),
                _(L"Save Script"), wxYES_NO|wxICON_QUESTION) == wxYES)
            { Save(); }
        }
    SetText(m_defaultHeader);
    SetSelection(GetLastPosition(), GetLastPosition());
    SetModified(false);
    SetFocus();

    SetScriptFilePath(wxEmptyString);
    }

//-------------------------------------------------------------
bool CodeEditor::Open()
    {
    if (GetModify())
        {
        if (wxMessageBox(_(L"Do you wish to save your unsaved changes?"),
                _(L"Save Script"), wxYES_NO|wxICON_QUESTION) == wxYES)
            { Save(); }
        }
    wxFileDialog dialogOpen
            (this, _(L"Select Script to Open"),
            wxEmptyString, wxEmptyString,
            GetFileFilter(),
            wxFD_OPEN|wxFD_FILE_MUST_EXIST|wxFD_PREVIEW);
    if (dialogOpen.ShowModal() != wxID_OK)
        { return false; }
    const wxString filePath = dialogOpen.GetPath();

    wxWindowUpdateLocker noUpdates(this);
    ClearAll();
    LoadFile(filePath);
    SetSelection(0,0);
    SetScriptFilePath(filePath);

    return true;
    }

//-------------------------------------------------------------
bool CodeEditor::Save()
    {
    if (GetScriptFilePath().empty())
        {
        wxFileDialog dialogSave
                (this, _(L"Save Script As"),
                wxEmptyString, wxEmptyString,
                GetFileFilter(),
                wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

        if (dialogSave.ShowModal() != wxID_OK)
            { return false; }
        SetScriptFilePath(dialogSave.GetPath());
        }
    if (!SaveFile(GetScriptFilePath()) )
        {
        wxMessageBox(wxString::Format(_(L"Unable to save file \"%s\"."), GetScriptFilePath()), 
            _(L"Error"), wxOK|wxICON_EXCLAMATION);
        return false;
        }
    return true;
    }

//-------------------------------------------------------------
void CodeEditor::OnKeyDown(wxKeyEvent& event)
    {
    if (event.ControlDown() && event.GetKeyCode() == L'S')
        { Save(); }
    else if (event.ControlDown() && event.GetKeyCode() == L'O')
        { Open(); }
    else if (event.ControlDown() && event.GetKeyCode() == L'N')
        { New(); }
    else
        { event.Skip(); }
    }

//-------------------------------------------------------------
void CodeEditor::OnFind(wxFindDialogEvent &event)
    {
    const int flags = event.GetFlags();
    int searchFlags = 0;
    if (flags & wxFR_MATCHCASE)
        { searchFlags = searchFlags|wxSTC_FIND_MATCHCASE; }
    if (flags & wxFR_WHOLEWORD)
        { searchFlags = searchFlags|wxSTC_FIND_WHOLEWORD; }

    if (flags & wxFR_DOWN)
        { FindNext(event.GetFindString(), searchFlags); }
    else
        { FindPrevious(event.GetFindString(), searchFlags); }
    }

//-------------------------------------------------------------
void CodeEditor::FindPrevious(const wxString& textToFind, const int searchFlags /*= 0*/)
    {
    SearchAnchor();
    long selStart(0), selEnd(0);
    GetSelection(&selStart,&selEnd);
    int foundPos = SearchPrev(searchFlags, textToFind);
    if (foundPos == selStart && foundPos != 0)
        {
        SetSelection(foundPos-1,foundPos-1);
        SearchAnchor();
        foundPos = SearchPrev(searchFlags, textToFind);
        if (foundPos != wxSTC_INVALID_POSITION)
            {
            SetSelection(foundPos,foundPos+textToFind.length());
            EnsureCaretVisible();
            }
        else
            { SetSelection(selStart, selEnd); }
        }
    else if (foundPos != wxSTC_INVALID_POSITION)
        {
        SetSelection(foundPos,foundPos+textToFind.length());
        EnsureCaretVisible();
        }
    // not found going forward, so start from beginning and try from there
    else
        {
        wxMessageBox(_(L"No occurrences found."),
                _(L"Item Not Found"), wxOK|wxICON_INFORMATION);
        }
    }

//-------------------------------------------------------------
void CodeEditor::FindNext(const wxString& textToFind, const int searchFlags /*= 0*/)
    {
    SearchAnchor();
    long selStart, selEnd;
    GetSelection(&selStart,&selEnd);
    int foundPos = SearchNext(searchFlags, textToFind);
    if (foundPos == selStart)
        {
        SetSelection(foundPos+textToFind.length(),foundPos+textToFind.length());
        SearchAnchor();
        foundPos = SearchNext(searchFlags, textToFind);
        if (foundPos != wxSTC_INVALID_POSITION)
            {
            SetSelection(foundPos,foundPos+textToFind.length());
            EnsureCaretVisible();
            }
        else
            { SetSelection(selStart, selEnd); }
        }
    else if (foundPos != wxSTC_INVALID_POSITION)
        {
        SetSelection(foundPos,foundPos+textToFind.length());
        EnsureCaretVisible();
        }
    // not found going forward, so start from beginning and try from there
    else
        {
        foundPos = FindText(0, GetLength(), textToFind, searchFlags);
        if (foundPos != wxSTC_INVALID_POSITION)
            {
            SetSelection(foundPos,foundPos+textToFind.length());
            EnsureCaretVisible();
            }
        else
            {
            wxMessageBox(_(L"No occurrences found."),
                    _(L"Item Not Found"), wxOK|wxICON_INFORMATION);
            }
        }
    }

//-------------------------------------------------------------
void CodeEditor::AddFunctionsOrClasses(const NameList& functions)
    {
    for (const auto& func : functions)
        { m_libaryAndClassNames.insert(StripExtraInfo(func.c_str())); }
    }

//-------------------------------------------------------------
void CodeEditor::AddLibrary(const wxString& library, NameList& functions)
    {
    wxString functionString;
    wxString returnTypeStr;
    for (const auto& func : functions)
        {
        functionString += L" " + StripExtraInfo(func);
        returnTypeStr = GetReturnType(func);
        if (returnTypeStr.length())
            { m_libraryFunctionsWithReturnTypes.insert(
                std::make_pair(library + L"." + StripExtraInfo(func), returnTypeStr) ); }
        }
    m_libraryCollection.insert(std::make_pair(library, functionString) );
    m_libaryAndClassNames.insert(library);
    }

//-------------------------------------------------------------
void CodeEditor::AddClass(const wxString& theClass, NameList& functions)
    {
    wxString functionString;
    for (const auto& func : functions)
        { functionString += L" " + StripExtraInfo(func); }
    m_classCollection.insert(std::make_pair(theClass, functionString) );
    m_libaryAndClassNames.insert(theClass);
    }

//-------------------------------------------------------------
void CodeEditor::Finalize()
    {
    for (const auto& className : m_libaryAndClassNames)
        { m_libaryAndClassNamesStr += L" " + className; }
    SetKeyWords(1, m_libaryAndClassNamesStr);
    }

//-------------------------------------------------------------
wxString CodeEditor::StripExtraInfo(const wxString& function)
    {
    const int extraInfoStart = function.find_first_of(L"\t (");
    if (extraInfoStart != wxNOT_FOUND)
        { return function.Mid(0, extraInfoStart); }
    else
        { return function; }
    }

//-------------------------------------------------------------
wxString CodeEditor::GetReturnType(const wxString& function)
    {
    const int parenthesisStart = function.find(L"\t");
    if (parenthesisStart != wxNOT_FOUND)
        {
        wxString returnType = function.Mid(parenthesisStart);
        returnType.Trim(true); returnType.Trim(false);
        return returnType;
        }
    else
        { return wxEmptyString; }
    }

//-------------------------------------------------------------
bool CodeEditor::SplitFunctionAndParams(wxString& function, wxString& params)
    {
    const int parenthesisStart = function.Find(L'(');
    if (parenthesisStart != wxNOT_FOUND)
        {
        const int parenthesisEnd = function.Find(L')', true);
        // if empty parameter list then don't bother splitting this up
        if (parenthesisEnd == parenthesisStart+1)
            { return false; }
        params = function.Mid(parenthesisStart+1,(parenthesisEnd-1)-parenthesisStart);
        function.Truncate(parenthesisStart);
        return true;
        }
    return false;
    }

//-------------------------------------------------------------
void CodeEditor::OnMarginClick(wxStyledTextEvent &event)
    {
    if (event.GetMargin() == 1)
        {
        const int lineClick = LineFromPosition(event.GetPosition());
        if ((GetFoldLevel(lineClick) & wxSTC_FOLDLEVELHEADERFLAG) > 0)
            { ToggleFold(lineClick); }
        }
    }

//-------------------------------------------------------------
void CodeEditor::OnCharAdded(wxStyledTextEvent &event)
    {
    if (event.GetKey() == GetLibraryAccessor())
        {
        const int wordStart = WordStartPosition(GetCurrentPos()-1, true);
        const wxString lastWord = GetTextRange(wordStart, GetCurrentPos()-1);

        auto pos = m_libraryCollection.find(lastWord);
        if (pos != m_libraryCollection.cend())
            { AutoCompShow(0, pos->second); }
        }
    else if (event.GetKey() == L')' || event.GetKey() == L'(')
        { CallTipCancel(); }
    else if (event.GetKey() == GetObjectAccessor())
        {
        int wordStart = WordStartPosition(GetCurrentPos()-1, false);
        const wxString lastWord = GetTextRange(wordStart, GetCurrentPos()-1);

        if (lastWord == L"()")
            {
            wordStart = WordStartPosition(wordStart-1, false);
            const wxString functionName = GetTextRange(wordStart, GetCurrentPos()-3);
            wordStart = WordStartPosition(wordStart-1, false);
            const wxString libraryName = GetTextRange(wordStart, GetCurrentPos()-3);
            auto libraryPos = m_libraryFunctionsWithReturnTypes.find(libraryName);
            if (libraryPos != m_libraryFunctionsWithReturnTypes.cend())
                {
                libraryPos = m_classCollection.find(libraryPos->second);
                if (libraryPos != m_classCollection.cend())
                    { AutoCompShow(0, libraryPos->second); }
                }
            }
        // might be a variable, look for where it was first assigned to something
        int foundPos = 0;
        while (foundPos+lastWord.length()+2 < static_cast<size_t>(wordStart))
            {
            foundPos = FindText(foundPos, wordStart, lastWord,
                                wxSTC_FIND_WHOLEWORD|wxSTC_FIND_MATCHCASE);
            if (foundPos != wxSTC_INVALID_POSITION &&
                foundPos+lastWord.length()+2 < static_cast<size_t>(wordStart))
                {
                foundPos += lastWord.length();
                while (foundPos < GetLength() && GetCharAt(foundPos) == L' ')
                    { ++foundPos; }
                // found an assignment to this variable
                if (foundPos < GetLength() && GetCharAt(foundPos) == L'=')
                    {
                    // scan to whatever it is assigned to
                    do
                        { ++foundPos; }
                    while (foundPos < GetLength() && GetCharAt(foundPos) == L' ');
                    // if it is a known class of ours, then show the functions
                    // available for that class
                    wxString assignment = GetTextRange(foundPos, WordEndPosition(foundPos, true));
                    const auto classPos = m_classCollection.find(assignment);
                    if (classPos != m_classCollection.cend())
                        {
                        AutoCompShow(0, classPos->second);
                        break;
                        }
                    else
                        { continue; }
                    }
                else
                    { continue; }
                }
            else
                { break; }
            }
        }
    else
        {
        const int wordStart = WordStartPosition(GetCurrentPos(), true);
        const wxString lastWord = GetTextRange(wordStart, GetCurrentPos());

        if (lastWord.length())
            {
            // see if we are inside a library, if so show its list of functions
            if (wordStart > 2 && GetCharAt(wordStart-1) == GetLibraryAccessor())
                {
                const wxString libraryName =
                    GetTextRange(WordStartPosition(wordStart-2, true), wordStart-1);
                auto pos = m_libraryCollection.find(libraryName);
                if (pos != m_libraryCollection.cend())
                    {
                    if (AutoCompActive())
                        { AutoCompSelect(lastWord); }
                    else
                        { AutoCompShow(lastWord.length(), pos->second); }
                    }
                }
            // if an object...
            else if (wordStart > 2 && GetCharAt(wordStart-1) == GetObjectAccessor())
                {
                int previousWordStart = WordStartPosition(wordStart-2, false);
                const wxString previousWord = GetTextRange(previousWordStart, wordStart-1);

                // see if it is an object returned from a known function
                if (previousWord == L"()")
                    {
                    previousWordStart = WordStartPosition(previousWordStart-1, false);
                    const wxString functionName = GetTextRange(previousWordStart, wordStart-1);
                    previousWordStart = WordStartPosition(previousWordStart-1, false);
                    const wxString libraryName = GetTextRange(previousWordStart, wordStart-1);
                    auto libraryPos = m_libraryFunctionsWithReturnTypes.find(libraryName);
                    if (libraryPos != m_libraryFunctionsWithReturnTypes.cend())
                        {
                        libraryPos = m_classCollection.find(libraryPos->second);
                        if (libraryPos != m_classCollection.cend())
                            {
                            if (AutoCompActive())
                                { AutoCompSelect(lastWord); }
                            else
                                { AutoCompShow(lastWord.length(), libraryPos->second); }
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
                    SplitFunctionAndParams(foundKeyword,params);
                    }
                // if found a full keyword, then just fix its case and let it auto-highlight
                if (pos != m_libaryAndClassNames.cend() &&
                    foundKeyword.length() == lastWord.length())
                    {
                    SetSelection(wordStart,wordStart+lastWord.length());
                    // tooltip the parameters (if applicable)
                    if (params.length())
                        {
                        foundKeyword += L"(";
                        ReplaceSelection(foundKeyword);
                        SetSelection(wordStart+foundKeyword.length(),
                                     wordStart+foundKeyword.length());
                        params += L")";
                        CallTipShow(GetCurrentPos(), params);
                        }
                    else
                        {
                        ReplaceSelection(foundKeyword);
                        SetSelection(wordStart+foundKeyword.length(),
                                     wordStart+foundKeyword.length());
                        }
                    AutoCompCancel();
                    }
                // or if a partial find, then show auto-completion
                else if (pos != m_libaryAndClassNames.end())
                    {
                    if (AutoCompActive())
                        { AutoCompSelect(lastWord); }
                    else
                        { AutoCompShow(lastWord.length(), m_libaryAndClassNamesStr); }
                    }
                else
                    { AutoCompCancel(); }
                }
            }
        else
            { AutoCompCancel(); }
        }
    event.Skip();
    }

//-------------------------------------------------------------
void CodeEditor::OnAutoCompletionSelected(wxStyledTextEvent &event)
    {
    wxString selectedString = event.GetText();
    wxString paramText;
    const bool hasParams = SplitFunctionAndParams(selectedString, paramText);

    const int wordStart = WordStartPosition(GetCurrentPos(), true);
    const int wordEnd = WordEndPosition(GetCurrentPos(), true);
    SetSelection(wordStart,wordEnd);
    if (hasParams)
        { selectedString += L"("; }
    ReplaceSelection(selectedString);
    SetSelection(wordStart+selectedString.length(), wordStart+selectedString.length());
    AutoCompCancel();
    if (hasParams)
        {
        paramText += L")";
        CallTipShow(GetCurrentPos(), paramText);
        }
    }
