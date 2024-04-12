/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __BASEMAINFRAME_H__
#define __BASEMAINFRAME_H__

#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcgraph.h>
#include <wx/dnd.h>
#include <wx/docview.h>
#include <wx/event.h>
#include <wx/filename.h>
#include <wx/paper.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/ribbon/gallery.h>
#include <wx/ribbon/toolbar.h>
#include <wx/wx.h>
#include <wx/xrc/xh_bmp.h>
#include <wx/xrc/xh_menu.h>
#include <wx/xrc/xh_statbar.h>
#include <wx/xrc/xh_toolb.h>
#include <wx/xrc/xmlres.h>

namespace Wisteria::UI
    {
    /// @brief Document manager class with a built-in document template selector
    ///     when the client fires a new document event.
    class DocManager final : public wxDocManager
        {
      public:
        /// @private
        [[nodiscard]]
        wxDocTemplate* SelectDocumentType(wxDocTemplate** templates, int noTemplates,
                                          bool sortDocs = false);
        };

    /// @brief Single-document application parent frame with built-in ribbon,
    ///     multi-document template, and help support.
    /// @details This is the initial application frame from which children frames are spawned from
    ///     and managed. This is the main (top-level) window of the application.\n
    ///     \n
    ///     Regarding the help system, this is designed for a folder containing "raw" help files,
    ///     such as a folder of HTML files and images. This folder path is defined by calling
    ///     SetHelpDirectory(), and then the default behaviour is to launch HTML files from
    ///     that folder. Help events (by default) will launch the file "index.html" from the
    ///     help folder into a browser, and DisplayHelp() will open a provided topic (by name)
    ///     from the same folder.
    class BaseMainFrame : public wxDocParentFrame
        {
      public:
        /// @brief Constructor.
        /// @param manager The document manager.
        /// @param frame The parent application frame.
        /// @param defaultFileExtentions The file types supported by the app.
        ///     If more than one file extension, then a selection dialog is shown when the
        ///     client fires a new document event.
        /// @param title The title of the frame.
        /// @param pos The position of the frame.
        /// @param size The size of the frame.
        /// @param style The window style.
        BaseMainFrame(wxDocManager* manager, wxFrame* frame,
                      const wxArrayString& defaultFileExtentions, const wxString& title,
                      const wxPoint& pos, const wxSize& size, long style);
        /// @private
        BaseMainFrame(const BaseMainFrame&) = delete;
        /// @private
        BaseMainFrame& operator=(const BaseMainFrame&) = delete;

        /// @brief Initializes the main sizer and (optionally) the ribbon.
        /// @param ribbon The ribbon to add to the main frame, or nullptr to not include a ribbon.
        void InitControls(wxRibbonBar* ribbon);

        /// @brief Connects DisplayHelp() to various help events, opening the default topic.
        /// @details Override this to call a different function.
        /// @param event The help event.
        virtual void OnHelpContents([[maybe_unused]] wxCommandEvent& event);
        /// @brief Opens the default help topic ("index.html" in the defined help folder).
        /// @param topic The name of the file to display.
        /// @sa SetHelpDirectory().
        void DisplayHelp(const wxString& topic = wxEmptyString);

        /// @private
        wxDocument* OpenFile(const wxString& path);
        /// @private
        void OpenFileNew(const wxString& path);

        /// @brief Holds printer settings for use globally.
        /// @param printData The printer settings to point to.
        /// @note Does not own this, it just receives it from the application framework
        void SetPrintData(wxPrintData* printData) noexcept { m_printData = printData; }

        /// @returns The list of file types (extensions) that the app supports.
        [[nodiscard]]
        const wxArrayString& GetDefaultFileExtentions() const noexcept
            {
            return m_defaultFileExtentions;
            }

        /// @brief Sets the list of file types (extensions) that the app supports.
        /// @details These are what are shown when the client fires a new document event
        ///     (if there are more than one file extension).
        /// @param extensions The supported extensions.
        void SetDefaultFileExtentions(const wxArrayString& extensions)
            {
            m_defaultFileExtentions = extensions;
            }

        /// @returns The program's logo.
        [[nodiscard]]
        const wxBitmap& GetLogo() const noexcept
            {
            return m_logo;
            }

        /// @brief Sets the program's logo.
        /// @param logo The logo for the program.
        void SetLogo(const wxBitmap& logo) { m_logo = logo; }

        /// @brief Sets the directory where the documentation is stored.
        /// @param helpFolder The help folder.
        /// @details When a help event if fired, "index.html" in this folder will
        ///     be opened in the system's default browser.
        void SetHelpDirectory(const wxString& helpFolder) { m_helpFolder = helpFolder; }

        /// @returns The directory where the documentation is stored.
        [[nodiscard]]
        const wxString& GetHelpDirectory() const noexcept
            {
            return m_helpFolder;
            }

        /// @returns The ribbon control.
        /// @note Will be null if not in use.
        [[nodiscard]]
        wxRibbonBar* GetRibbon() noexcept
            {
            return m_ribbon;
            }

      private:
        // Ribbon features
        /// @private
        void OnRibbonButtonBarClick(wxRibbonButtonBarEvent& evt);
        /// @private
        void OnRibbonToolBarClick(wxRibbonToolBarEvent& evt);

        wxRibbonBar* m_ribbon{ nullptr };
        wxPrintData* m_printData{ nullptr };
        wxString m_helpFolder;
        wxArrayString m_defaultFileExtentions;
        wxBitmap m_logo;
        wxDECLARE_CLASS(BaseMainFrame);
        };

    /// @brief drag 'n' drop file support for the mainframe.
    class DropFiles final : public wxFileDropTarget
        {
      public:
        /// @brief Constructor.
        /// @param frame The main frame to connect the D'n'D support to.
        explicit DropFiles(BaseMainFrame* frame) : m_frame(frame) {}

        /// @private
        bool OnDropFiles([[maybe_unused]] wxCoord x, [[maybe_unused]] wxCoord y,
                         const wxArrayString& filenames) final;

      private:
        BaseMainFrame* m_frame{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif //__BASEMAINFRAME_H__
