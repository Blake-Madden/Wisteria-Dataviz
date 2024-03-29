/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __BASEAPP_H__
#define __BASEAPP_H__

#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/utils.h>
#include <wx/mimetype.h>
#include <wx/docview.h>
#include <wx/fs_zip.h>
#include <wx/sysopt.h>
#include <wx/splash.h>
#include <wx/image.h>
#include <wx/fileconf.h>
#include <wx/cmdline.h>
#include <wx/debugrpt.h>
#include <wx/log.h>
#include <wx/uilocale.h>
#include <wx/fs_mem.h>
#include <wx/config.h>
#include <wx/propgrid/propgrid.h>
#include <wx/webrequest.h>
#include "../util/logfile.h"
#include "../util/resource_manager.h"
#include "../i18n-check/src/donttranslate.h"
#include "../debug/debug_profile.h"
#include "../math/safe_math.h"
#include "mainframe.h"

namespace Wisteria::UI
    {
    /// @brief Application class with file history, file logger, profiler,
    ///      exception handling, document manager, and ribbon-based
    ///     main frame built-in.
    class BaseApp : public wxApp
        {
    public:
        /// @brief Constructor.
        BaseApp();
        /// @private
        BaseApp(const BaseApp&) = delete;
        /// @private
        BaseApp& operator=(const BaseApp&) = delete;

        /// @private
        bool OnInit() override;
        /// @private
        int OnExit() override;

        /// @private
        void OnFatalException() final;
        // this is where we really generate the debug report
        /// @private
        void GenerateReport(wxDebugReport::Context ctx);

        /// @returns The number of active documents.
        [[nodiscard]]
        size_t GetDocumentCount() const
            { return m_docManager->GetDocuments().GetCount(); }

        /// @returns The application's main file extension.
        [[nodiscard]]
        const wxString& GetAppFileExtension() const noexcept
            { return m_fileExtension; }
        /// @brief Sets the application's main file extension.
        /// @param extension The file extension to use.
        void SetAppFileExtension(const wxString& extension)
            { m_fileExtension = extension; }

        // MRU menu functions
        /// @private
        void LoadFileHistoryMenu();
        /// @private
        void SaveFileHistoryMenu();
        /// @brief Clears the MRU file history.
        virtual void ClearFileHistoryMenu();

        /// @returns The mainframe.
        [[nodiscard]]
        BaseMainFrame* GetMainFrame() noexcept
            { return m_mainFrame; }
        /// @private
        [[nodiscard]]
        const BaseMainFrame* GetMainFrame() const noexcept
            { return m_mainFrame; }
        /// @brief Sets the mainframe.
        /// @param frame The mainframe to use.
        void SetMainFrame(BaseMainFrame* frame)
            {
            m_mainFrame = frame;
            SetTopWindow(m_mainFrame);
            }

        /// @returns The document manager.
        [[nodiscard]]
        wxDocManager* GetDocManager() noexcept
            { return m_docManager; }
        /// @brief Sets the document manager.
        /// @param docManager The document manager to use.
        void SetDocManager(wxDocManager* docManager) noexcept
            { m_docManager = docManager; }

        /// @brief Sets a descriptive name for the application's document type.
        /// @param documentTypeName The descriptives.
        void SetDocumentTypeName(const wxString& documentTypeName)
            { m_documentTypeName = documentTypeName; }

        /// @returns The application's document version number.
        [[nodiscard]]
        const wxString& GetDocumentVersionNumber() const noexcept
            { return m_documentVersionNumber; }
        /// @brief Sets the application's document version number.
        /// @param versionNumber The version number.
        void SetDocumentVersionNumber(const wxString& versionNumber)
            { m_documentVersionNumber = versionNumber; }

        /// @returns The application's subname.
        [[nodiscard]]
        const wxString& GetAppSubName() const noexcept
            { return m_appSubName; }
        /// @brief Sets the application's subname.
        /// @param name The subname.
        void SetAppSubName(const wxString& name)
            { m_appSubName = name; }

        /// @returns The path of where the debug profiling data is being saved.
        /// @details This is only used if profiling is enabled.
        [[nodiscard]]
        const wxString& GetProfileReportPath() const noexcept
            { return m_profileReportPath; }

        /// @returns Everything sent to the logging system as a formatted string.
        [[nodiscard]]
        wxString GetLogReport()
            { return m_logFile->Read(); }

        /// @return The file logging systems used by the application.
        [[nodiscard]]
        LogFile* GetLogFile() const noexcept
            {
            return m_logFile;
            }

        /// @brief Sets a support email for the application.
        /// @param email The email address.
        void SetSupportEmail(const wxString& email)
            { m_supportEmail = email; }

        /// @returns The printer settings.
        [[nodiscard]]
        wxPrintData* GetPrintData()
            {
            if (GetMainFrame())
                {
                return &GetMainFrame()->GetDocumentManager()->
                    GetPageSetupDialogData().GetPrintData();
                }
            else
                { return nullptr; }
            }

        /// @returns The path to a file in the application's resource directory,
        ///     or empty string if not found.
        /// @param subFile The file to look for.
        [[nodiscard]]
        wxString FindResourceFile(const wxString& subFile) const;

        /// @returns A subdirectory's full path in the application's resource directory,
        ///     or empty string if not found.
        /// @param subDir The subdirectory to look for.
        [[nodiscard]]
        wxString FindResourceDirectory(const wxString& subDir) const;

        /// @returns The resource manager, which can extract images and
        ///     XRC files from a resource archive.
        [[nodiscard]]
        ResourceManager& GetResourceManager() noexcept
            { return m_resManager; }

        /// @private
        [[nodiscard]]
        const ResourceManager& GetResourceManager() const noexcept
            { return m_resManager; }

        /** @brief Creates a program's splashscreen using a base image and
                various program information.
            @param bitmap The base image.
            @param appName The application's name.
            @param appSubName The application's supplemental name
                (e.g., the version number).
            @param vendorName The application's vendor.
            @param includeCopyright Whether to show a copyright label at the
                bottom of the splashscreen.
            @returns The decorated splashscreen.*/
        [[nodiscard]]
        wxBitmap CreateSplashscreen(const wxBitmap& bitmap, const wxString& appName,
            const wxString& appSubName, const wxString& vendorName,
            const bool includeCopyright);
    private:
        [[nodiscard]]
        wxString FindResourceFileWithAppInfo(const wxString& folder,
                                             const wxString& subFile) const;
        [[nodiscard]]
        wxString FindResourceDirectoryWithAppInfo(const wxString& folder,
                                                  const wxString& subFile) const;

        wxDocManager* m_docManager{ nullptr };
        Wisteria::UI::BaseMainFrame* m_mainFrame{ nullptr };
        LogFile* m_logFile{ nullptr };
        ResourceManager m_resManager;

        wxString m_appSubName;
        wxString m_fileExtension;
        wxString m_documentTypeName;
        wxString m_documentVersionNumber;
        wxString m_profileReportPath;
        wxString m_supportEmail;
        };
    }

/** @}*/

#endif //__BASEAPP_H__
