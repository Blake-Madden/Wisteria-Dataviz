/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef FUNCTION_BROWSER_DLG_H
#define FUNCTION_BROWSER_DLG_H

#include "../../import/html_encode.h"
#include "../../import/html_extract_text.h"
#include "../../util/string_util.h"
#include "../controls/sidebar.h"
#include "dialogwithhelp.h"
#include <set>
#include <utility>
#include <vector>
#include <wx/html/htmlwin.h>
#include <wx/listbox.h>
#include <wx/stc/stc.h>
#include <wx/tokenzr.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Control containing a library/class list, respective function list,
            and dynamic description window.*/
    class FunctionBrowserCtrl : public wxControl
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param editor The editor (i.e., text editor) that this dialog can insert text into.
            @param id The dialog's ID.
            @param firstWindowCaption The caption above the categories list.
            @param secondWindowCaption The caption above the list of functions.*/
        FunctionBrowserCtrl(wxWindow* parent, wxWindow* editor, wxWindowID id = wxID_ANY,
                            const wxString& firstWindowCaption = _(L"Categories:"),
                            const wxString& secondWindowCaption = _(L"Functions/Operators:"))
            : wxControl(parent, id), m_editWindow(editor)
            {
            CreateControls(firstWindowCaption, secondWindowCaption);
            }

        /// @private
        FunctionBrowserCtrl() = delete;
        /// @private
        FunctionBrowserCtrl(const FunctionBrowserCtrl&) = delete;
        /// @private
        FunctionBrowserCtrl& operator=(const FunctionBrowserCtrl&) = delete;

        /// @brief Container type for function, class, and category names.
        using NameList = std::set<std::wstring, string_util::string_no_case_less>;

        /** @brief Adds a category that doesn't contain functions, but rather other categories.
            @param category The category label.
            @param Id The ID for the category. This ID is used to connect subcategories to this one.
            @param iconIndex The icon to show next to the category label. @c -1 means no icon.*/
        void AddCategory(const wxString& category, const wxWindowID Id, const long iconIndex = -1)
            {
            m_categoryList->InsertItem(m_categoryList->GetFolderCount(), category, Id, iconIndex);
            }

        /** @brief Adds a category and its associated functions.
            @param category The category label.
            @param functions The functions to associate with the category.\n
                Each item in this array can be either:
                - a tab-delimited string containing the function signature, description,
                  and return type (optional).
                - a string containing function signature, "->" and the return type (can be empty).
            @param parentId The ID of the parent category to attach this to category under.
                If @c wxID_ANY, then this will be a root-level category.
            @param iconIndex The icon to show next to the category label.
                @c -1 means no icon.*/
        void AddCategory(const std::wstring& category,
                         const std::set<std::wstring, string_util::string_no_case_less>& functions,
                         const wxWindowID parentId = wxID_ANY, const int iconIndex = -1)
            {
            m_functionCollection.insert({ category, functions, parentId, iconIndex });
            }

        /** @brief Organizes all of the categories and subcategories.
            @details Call this after the final call to AddCategory().*/
        void FinalizeCategories();

        /** @returns The image list*/
        [[nodiscard]]
        std::vector<wxBitmapBundle>& GetImageList() noexcept
            {
            return m_categoryList->GetImageList();
            }

        /// @returns The sidebar.
        [[nodiscard]]
        Wisteria::UI::SideBar* GetSidebar() noexcept
            {
            return m_categoryList;
            }

        /// @brief Sets the editor that the control is pointing to.
        /// @param editor The editor window.
        void SetEditor(wxWindow* editor) { m_editWindow = editor; }

        /** @brief Specifies the separator used between function arguments
                (usually a comma, sometimes a semicolon for other regions.
                The default is a comma.
            @param sep The parameter separator.*/
        void SetParameterSeparator(const wchar_t sep) noexcept { m_paramSeparator = sep; }

        /// @brief Inserts the selected item in the function list into the buddy editor window.
        void InsertFunction();

      private:
        constexpr static int ID_CATEGORY_LIST = wxID_HIGHEST;
        constexpr static int ID_FUNCTION_LIST = wxID_HIGHEST + 1;

        /** @brief Creates the class/function lists and description window controls.*/
        void CreateControls(const wxString& firstWindowCaption,
                            const wxString& secondWindowCaption);

        [[nodiscard]]
        wxString FormatFunctionSignature(wxString signature) const;

        [[nodiscard]]
        static wxString GetFunctionName(const wxString& signature)
            {
            const auto firstParam = signature.find(L'(');
            return (firstParam == wxString::npos) ? signature : signature.substr(0, firstParam);
            }

        [[nodiscard]]
        bool SplitFunctionAndParams(wxString& function, wxString& params)
            {
            const auto parenthesisStart = function.find(L'(');
            if (parenthesisStart != wxString::npos)
                {
                const auto parenthesisEnd = function.find(L')', true);
                // if empty parameter list then don't bother splitting this up
                if (parenthesisEnd == parenthesisStart + 1)
                    {
                    return false;
                    }
                params =
                    function.substr(parenthesisStart + 1, (parenthesisEnd - 1) - parenthesisStart);
                function.Truncate(parenthesisStart);
                return true;
                }
            return false;
            }

        void OnListSelected(wxCommandEvent& event);
        void OnHyperlinkClicked(const wxHtmlLinkEvent& event);

        struct CategoryInfo
            {
            explicit CategoryInfo(const std::wstring& name) : m_name(name) {}

            CategoryInfo(const std::wstring& name, const NameList& functions,
                         const wxWindowID parentId, const int iconIndex)
                : m_name(name), m_functions(functions), m_parentId(parentId), m_iconIndex(iconIndex)
                {
                }

            CategoryInfo() = default;

            [[nodiscard]]
            bool operator<(const CategoryInfo& that) const noexcept
                {
                return string_util::stricmp(m_name.c_str(), that.m_name.c_str()) < 0;
                }

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
        };

    /** @brief A function/object model browsing dialog.
        @details This is a dialog that wraps a FunctionBrowserCtrl and adds a set of
            "Insert," "Close," and "Help" buttons.
    @par Example:
    @code
        FunctionBrowserCtrl::NameList ApplicationObjectFunctions, StandardProjectFunctions,
                              BatchProjectFunctions, ListTypeEnum;
        StandardProjectFunctions.insert(L"Open()\tOpens a project.\tStandardProject");
        functionBrowser = new FunctionBrowserDlg(this, scriptCtrl, wxID_ANY,
                _(L"Library Browser"), _(L"Libraries/Classes"), _(L"Functions"),
                Colors::ColorBrewer::GetColor(Colors::Color::Blue));
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(L"Libraries", 1000);
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(L"Classes", 1001);
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(L"Enumerations", 1002);
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(
            L"Application", ApplicationObjectFunctions, 1000);
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(
            L"StandardProject", StandardProjectFunctions, 1001);
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(
            L"BatchProject", BatchProjectFunctions, 1001);
        functionBrowser->GetFunctionBrowserCtrl()->AddCategory(
            L"ListType", ListTypeEnum, 1002);
        functionBrowser->GetFunctionBrowserCtrl()->FinalizeCategories();
    @endcode*/
    class FunctionBrowserDlg final : public Wisteria::UI::DialogWithHelp
        {
      public:
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
        FunctionBrowserDlg(wxWindow* parent, wxWindow* editor, wxWindowID id = wxID_ANY,
                           const wxString& caption = _(L"Function Browser"),
                           const wxString& firstWindowCaption = _(L"Categories:"),
                           const wxString& secondWindowCaption = _(L"Functions/Operators:"),
                           const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER)
            {
            Create(parent, editor, id, caption, firstWindowCaption, secondWindowCaption, pos, size,
                   style);
            }

        /// @private
        FunctionBrowserDlg() = default;
        /// @private
        FunctionBrowserDlg(const FunctionBrowserDlg& that) = delete;
        /// @private
        FunctionBrowserDlg& operator=(const FunctionBrowserDlg& that) = delete;
        /** @brief Create function (use if constructed with an empty constructor).
            @param parent The parent window.
            @param editor The editor (i.e., text editor) that this dialog can insert text into.
            @param id The dialog's ID.
            @param caption The dialog's caption.
            @param firstWindowCaption The caption above the categories list.
            @param secondWindowCaption The caption above the list of functions.
            @param pos The dialog's position.
            @param size The dialog's size.
            @param style The dialog's window style.
            @note Using this will not connect this browser to an editor.
            @returns @c true if successfully created.*/
        bool Create(wxWindow* parent, wxWindow* editor, wxWindowID id = wxID_ANY,
                    const wxString& caption = _(L"Function Browser"),
                    const wxString& firstWindowCaption = _(L"Categories:"),
                    const wxString& secondWindowCaption = _(L"Functions/Operators:"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @returns The function browser control.
        [[nodiscard]]
        FunctionBrowserCtrl* GetFunctionBrowserCtrl() noexcept
            {
            return m_funcBrowserControl;
            }

      private:
        constexpr static int ID_INSERT_BUTTON = wxID_HIGHEST + 2;

        FunctionBrowserCtrl* m_funcBrowserControl{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // FUNCTION_BROWSER_DLG_H
