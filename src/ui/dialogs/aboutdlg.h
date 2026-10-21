/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef ABOUT_DLG_H
#define ABOUT_DLG_H

#include "../controls/sidebarbook.h"
#include "dialogwithhelp.h"
#include <vector>
#include <wx/bmpbndl.h>
#include <wx/string.h>
#include <wx/versioninfo.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief How a citation is displayed and copied.
    enum class CitationFormat
        {
        /// @brief Shown as HTML. Copied with the markup removed.
        Html,
        /// @brief Shown in a monospace text box. Copied as-is.
        PlainText
        };

    /// @brief A citation shown on the Citation page.
    struct AboutCitation
        {
        /// @brief The label shown above the citation (e.g., "MLA:").
        wxString m_label;
        /// @brief The citation's content.
        wxString m_text;
        /// @brief How the citation is displayed and copied.
        CitationFormat m_format{ CitationFormat::Html };
        };

    /// @brief A third-party component listed on the SBOM page.
    struct AboutLibraryInfo
        {
        /// @brief The library's name, version, and copyright.
        wxVersionInfo m_versionInfo;
        /// @brief The commit hash that the library was built from.
        wxString m_commitHash;
        /// @brief The library's repository URL.
        wxString m_origin;
        };

    /// @brief Content shown in an AboutDlg.
    class AboutDlgInfo
        {
        friend class AboutDlg;

      public:
        /// @brief Sets the application's name.
        /// @param appName The name.
        /// @returns A self reference.
        AboutDlgInfo& AppName(wxString appName)
            {
            m_appName = std::move(appName);
            return *this;
            }

        /// @brief Sets the application's version.
        /// @param appVersion The version.
        /// @returns A self reference.
        AboutDlgInfo& AppVersion(wxString appVersion)
            {
            m_appVersion = std::move(appVersion);
            return *this;
            }

        /// @brief Sets the build version. The "Build" row is hidden if empty.
        /// @param buildVersion The build version.
        /// @returns A self reference.
        AboutDlgInfo& BuildVersion(wxString buildVersion)
            {
            m_buildVersion = std::move(buildVersion);
            return *this;
            }

        /// @brief Sets the application's logo.
        /// @param logo The logo.
        /// @returns A self reference.
        AboutDlgInfo& Logo(wxBitmapBundle logo)
            {
            m_logo = std::move(logo);
            return *this;
            }

        /// @brief Sets the copyright notice.
        /// @param copyright The copyright notice.
        /// @returns A self reference.
        AboutDlgInfo& Copyright(wxString copyright)
            {
            m_copyright = std::move(copyright);
            return *this;
            }

        /// @brief Sets the license text. The License page is hidden if empty.
        /// @param eula The license text.
        /// @returns A self reference.
        AboutDlgInfo& Eula(wxString eula)
            {
            m_eula = std::move(eula);
            return *this;
            }

        /// @brief Adds a citation. Blank citations are ignored.
        /// @param label The label shown above the citation (e.g., "MLA:").
        /// @param text The citation's content.
        /// @param format How the citation is displayed and copied.
        /// @returns A self reference.
        AboutDlgInfo& AddCitation(wxString label, wxString text,
                                  CitationFormat format = CitationFormat::Html);

        /// @brief Adds a citation loaded from a file.
        /// @details Files that are missing, unreadable, or blank are ignored.
        /// @param label The label shown above the citation (e.g., "MLA:").
        /// @param filePath The file to read the citation from.
        /// @param format How the citation is displayed and copied.
        /// @returns A self reference.
        AboutDlgInfo& AddCitationFile(wxString label, wxString filePath,
                                      CitationFormat format = CitationFormat::Html);

        /// @brief Adds a library to the SBOM page. The page is hidden if no libraries are added.
        /// @param versionInfo The library's name, version, and copyright.
        /// @param commitHash The commit hash that the library was built from.
        /// @param origin The library's repository URL.
        /// @returns A self reference.
        AboutDlgInfo& AddLibrary(wxVersionInfo versionInfo, wxString commitHash = wxString{},
                                 wxString origin = wxString{})
            {
            m_libraries.push_back(
                { std::move(versionInfo), std::move(commitHash), std::move(origin) });
            return *this;
            }

      private:
        wxString m_appName;
        wxString m_appVersion;
        wxString m_buildVersion;
        wxBitmapBundle m_logo;
        wxString m_copyright;
        wxString m_eula;
        std::vector<AboutCitation> m_citations;
        std::vector<AboutLibraryInfo> m_libraries;
        };

    /// @brief About dialog showing product info, license, SBOM, and citations.
    class AboutDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The dialog's parent.
            @param info The content to display.
            @param id The dialog's ID.
            @param pos The dialog's position.
            @param size The dialog's size.
            @param style The dialog's style.*/
        AboutDlg(wxWindow* parent, AboutDlgInfo info, wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                 long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        /// @private
        AboutDlg(const AboutDlg&) = delete;
        /// @private
        AboutDlg& operator=(const AboutDlg&) = delete;

        /// @returns The sidebar.
        [[nodiscard]]
        SideBar* GetSideBar() noexcept
            {
            return m_sideBarBook->GetSideBar();
            }

      private:
        constexpr static int ID_VERSION_PAGE = wxID_HIGHEST;
        constexpr static int ID_LICENSING_PAGE = wxID_HIGHEST + 1;
        constexpr static int ID_CITATION_PAGE = wxID_HIGHEST + 2;
        constexpr static int ID_COMPONENTS_PAGE = wxID_HIGHEST + 3;
        constexpr static int ID_COPY_COMPONENTS = wxID_HIGHEST + 4;
        constexpr static int ID_COPY_LICENSE = wxID_HIGHEST + 5;
        constexpr static int ID_COPY_PRODUCT_INFO = wxID_HIGHEST + 6;

        bool Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                    long style);
        void CreateControls();
        static void CopyToClipboard(const wxString& source, bool stripHtml);

        SideBarBook* m_sideBarBook{ nullptr };

        AboutDlgInfo m_info;
        wxString m_components;
        wxString m_productInfo;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // ABOUT_DLG_H
