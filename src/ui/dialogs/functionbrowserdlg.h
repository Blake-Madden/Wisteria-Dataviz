/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __FUNCTION_BROWSER_DLG_H__
#define __FUNCTION_BROWSER_DLG_H__

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/tokenzr.h>
#include <wx/html/htmlwin.h>
#include <wx/stc/stc.h>
#include <wx/wupdlock.h>
#include <vector>
#include <utility>
#include <set>
#include "dialogwithhelp.h"
#include "../controls/sidebar.h"
#include "../../i18n-check/src/string_util.h"
#include "../../import/html_encode.h"
#include "../../import/html_extract_text.h"

/** @brief A function/object model browsing dialog.
    @par Example:
    @code
        FunctionBrowserDlg::NameList ApplicationObjectFunctions, StandardProjectFunctions, 
                              BatchProjectFunctions, ListTypeEnum;
        StandardProjectFunctions.insert(L"Open()\tOpens a project.\tStandardProject");
        functionBrowser = new FunctionBrowserDlg(this, scriptCtrl, wxID_ANY,
                _("Library Browser"), _("Libraries/Classes"), _("Functions"), *wxBLUE);
        functionBrowser->AddCategory(L"Libraries", 1000);
        functionBrowser->AddCategory(L"Classes", 1001);
        functionBrowser->AddCategory(L"Enumerations", 1002);
        functionBrowser->AddCategory(L"Application", ApplicationObjectFunctions, 1000);
        functionBrowser->AddCategory(L"StandardProject", StandardProjectFunctions, 1001);
        functionBrowser->AddCategory(L"BatchProject", BatchProjectFunctions, 1001);
        functionBrowser->AddCategory(L"ListType", ListTypeEnum, 1002);
        functionBrowser->FinalizeCategories();
    @endcode
*/
class FunctionBrowserDlg final : public Wisteria::UI::DialogWithHelp
    {
public:
    /// @brief Container type for function, class, and category names.
    using NameList = std::set<std::wstring, string_util::string_no_case_less>;
    /** @brief Constructor.
        @param parent The parent window.
        @param editor The editor (i.e., text editor) that this dialog can insert text into.
        @param id The dialog's ID.
        @param caption The dialog's caption.
        @param firstWindowCaption The caption above the categories list.
        @param secondWindowCaption The caption above the list of functions.
        @param pos The dialog's position.
        @param size The dialog's size.
        @param style The dialog's window style.
        @note @c editor must be a `wxStyledTextCtrl`-derived window.*/
    FunctionBrowserDlg(wxWindow* parent,
                       wxWindow* editor,
                       wxWindowID id = wxID_ANY,
                       const wxString& caption = _("Function Browser"),
                       const wxString& firstWindowCaption = _("Categories:"),
                       const wxString& secondWindowCaption = _("Functions/Operators:"),
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER ) :
            m_editWindow(editor)
        { Create(parent, id, caption, firstWindowCaption, secondWindowCaption, pos, size, style); }
    /// @private
    FunctionBrowserDlg() = default;
    /// @private
    FunctionBrowserDlg(const FunctionBrowserDlg& that) = delete;
    /// @private
    FunctionBrowserDlg& operator=(const FunctionBrowserDlg& that) = delete;
    /** @brief Create function (use if constructed with an empty constructor).
        @param parent The parent window.
        @param id The dialog's ID.
        @param caption The dialog's caption.
        @param firstWindowCaption The caption above the categories list.
        @param secondWindowCaption The caption above the list of functions.
        @param pos The dialog's position.
        @param size The dialog's size.
        @param style The dialog's window style.
        @note Using this will not connect this browser to an editor.
        @returns @c true if successfully created.*/
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxString& caption = _("Function Browser"),
                const wxString& firstWindowCaption = _("Categories:"),
                const wxString& secondWindowCaption = _("Functions/Operators:"),
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER);
    /** @brief Adds a category that doesn't contain functions, but rather other categories
        @param category The category label.
        @param Id The ID for the category. This ID is used to connect subcategories to this one.
        @param iconIndex The icon to show next to the category label. -1 means no icon.*/
    void AddCategory(const wxString& category, const wxWindowID Id, const long iconIndex = -1)
        { m_categoryList->InsertItem(m_categoryList->GetFolderCount(), category, Id, iconIndex); }
    /** @brief Adds a category that doesn't contain functions, but rather other categories
        @param category The category label.
        @param functions The functions to associate with the category.\n
            Each item in this array should be a tab-delimited string containing
            the function name, description, and return type (optional).
        @param parentId The ID of the parent category to attach this to category under.
            If @c -1, then this will be a root-level category.
        @param iconIndex The icon to show next to the category label.
            @c -1 means no icon.*/
    void AddCategory(const std::wstring& category,
                     const std::set<std::wstring, string_util::string_no_case_less>& functions,
                     const wxWindowID parentId = wxID_ANY, const long iconIndex = -1)
        { m_functionCollection.insert({ category, functions, parentId, iconIndex }); }
    /** @brief Organizes all of the categories and subcategories.
        @details Call this after the final call to AddCategory().*/
    void FinalizeCategories();
    /** @returns The image list*/
    [[nodiscard]]
    std::vector<wxBitmapBundle>& GetImageList() noexcept
        { return m_categoryList->GetImageList(); }
    /// @returns The sidebar.
    [[nodiscard]]
    Wisteria::UI::SideBar* GetSidebar() noexcept
        { return m_categoryList; }

    /** @brief Specifies the separator used between function arguments
            (usually a comma, sometimes a semicolon for other regions.
            The default is a comma.
        @param sep The parameter separator.*/
    void SetParameterSeparator(const wchar_t sep) noexcept
        { m_paramSeparator = sep; }
private:
    static constexpr int ID_CATEGORY_LIST = wxID_HIGHEST;
    static constexpr int ID_FUNCTION_LIST = wxID_HIGHEST + 1;
    static constexpr int ID_INSERT_BUTTON = wxID_HIGHEST + 2;

    [[nodiscard]]
    wxString FormatFunctionSignature(wxString signature);
    [[nodiscard]]
    static wxString GetFunctionName(const wxString& signature)
        {
        const size_t firstParam = signature.find(wxT('('));
        return (firstParam == wxNOT_FOUND) ?
            signature : signature.Mid(0,firstParam);
        }
    [[nodiscard]]
    bool SplitFunctionAndParams(wxString& function, wxString& params)
        {
        const int parenthesisStart = function.find(L'(');
        if (parenthesisStart != wxNOT_FOUND)
            {
            const int parenthesisEnd = function.find(L')', true);
            // if empty parameter list then don't bother splitting this up
            if (parenthesisEnd == parenthesisStart + 1)
                { return false; }
            params = function.Mid(parenthesisStart + 1,(parenthesisEnd-1)-parenthesisStart);
            function.Truncate(parenthesisStart);
            return true;
            }
        return false;
        }
    void CreateControls(const wxString& firstWindowCaption, const wxString& secondWindowCaption);
    void OnListSelected(wxCommandEvent& event);
    void OnInsertButtonClick([[maybe_unused]] wxCommandEvent& event);
    void OnHyperlinkClicked(wxHtmlLinkEvent& event);
    void InsertFunction();

    struct CategoryInfo
        {
        CategoryInfo(const std::wstring& name) : m_name(name)
            {}
        CategoryInfo(const std::wstring& name,
                     const NameList& functions,
                     const wxWindowID parentId, const int iconIndex) :
            m_name(name), m_functions(functions), m_parentId(parentId), m_iconIndex(iconIndex)
            {}
        CategoryInfo() = default;
        [[nodiscard]]
        bool operator<(const CategoryInfo& that) const noexcept
            { return string_util::stricmp(m_name.c_str(), that.m_name.c_str()) < 0; };
        std::wstring m_name;
        NameList m_functions;
        wxWindowID m_parentId{ -1 };
        int m_iconIndex{ -1 };
        int m_lastSelectedItem{ -1 };
        };

    wchar_t m_paramSeparator{ L',' };
    NameList m_categoryNames;
    std::set<CategoryInfo> m_functionCollection;
    std::vector<std::pair<wxString, wxString>> m_currentFunctionsAndDescriptions;
    wxArrayString m_iconPaths;

    wxWindow* m_editWindow{ nullptr };
    Wisteria::UI::SideBar* m_categoryList{ nullptr };
    wxListBox* m_functionList{ nullptr };
    wxHtmlWindow* m_functionDescriptionWindow{ nullptr };

    wxDECLARE_CLASS(FunctionBrowserDlg);
    };

/** @}*/

#endif //__FUNCTION_BROWSER_DLG_H__
