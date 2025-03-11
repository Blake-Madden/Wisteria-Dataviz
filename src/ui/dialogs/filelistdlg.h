/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __FILELIST_DLG_H__
#define __FILELIST_DLG_H__

#include "../controls/listctrlex.h"
#include "../controls/thumbnail.h"
#include "dialogwithhelp.h"
#include <wx/filename.h>
#include <wx/infobar.h>
#include <wx/richmsgdlg.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog which displays a list of files.
    @details When selecting a file in the list, information (and thumbnail) about the file
        is displayed to the right.*/
    class FileListDlg final : public Wisteria::UI::DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param id The window ID.
            @param caption The title of the export dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        explicit FileListDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                             const wxString& caption = _(L"File List"),
                             const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                          wxRESIZE_BORDER)
            {
            SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS | wxWS_EX_CONTEXTHELP);
            Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style);

            CreateControls();
            Centre();
            }

        /// @private
        FileListDlg(const FileListDlg& that) = delete;
        /// @private
        FileListDlg& operator=(const FileListDlg& that) = delete;

        /// @returns Access to the file list control, which should be filled before calling
        /// ShowModal().
        [[nodiscard]]
        ListCtrlEx* GetListCtrl() noexcept
            {
            return m_listCtrl;
            }

        /// @returns Access to the file list control's backend data provider,
        ///     which should be filled before calling ShowModal().
        [[nodiscard]]
        const std::shared_ptr<ListCtrlExNumericDataProvider>& GetListCtrlData()
            {
            return m_fileData;
            }

        /// @returns Access to a descriptive infobar shown when the dialog is presented.
        [[nodiscard]]
        wxInfoBar* GetInforBar() noexcept
            {
            return m_infoBar;
            }

      private:
        void CreateControls();
        void BindEvents();

        constexpr static int ID_FOLDER_OPEN = wxID_HIGHEST;

        ListCtrlEx* m_listCtrl{ nullptr };
        std::shared_ptr<ListCtrlExNumericDataProvider> m_fileData{
            std::make_shared<ListCtrlExNumericDataProvider>()
        };
        Wisteria::UI::Thumbnail* m_thumbnail{ nullptr };
        wxStaticText* m_label{ nullptr };
        wxInfoBar* m_infoBar{ nullptr };
        };
    } // namespace Wisteria::UI

    /** @}*/

#endif //__FILELIST_DLG_H__
