/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef LIST_DIALOG_H
#define LIST_DIALOG_H

#include "../../util/logfile.h"
#include "../controls/listctrlex.h"
#include "../controls/searchpanel.h"
#include <wx/checklst.h>
#include <wx/dialog.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Which features to include for a ListDlg.
    enum ListDlgFlags
        {
        /// @brief No ribbon buttons.
        LD_NO_BUTTONS = 0,
        /// @brief A save button.
        LD_SAVE_BUTTON = 1,
        /// @brief A copy button.
        LD_COPY_BUTTON = 1 << 1,
        /// @brief A print button.
        LD_PRINT_BUTTON = 1 << 2,
        /// @brief A select all button.
        LD_SELECT_ALL_BUTTON = 1 << 3,
        /// @brief A find button.
        LD_FIND_BUTTON = 1 << 4,
        /// @brief An OK/cancel button.
        LD_OK_CANCEL_BUTTONS = 1 << 5,
        /// @brief A Yes/No button.
        LD_YES_NO_BUTTONS = 1 << 6,
        /// @brief Include column headers in the list control.
        LD_COLUMN_HEADERS = 1 << 7,
        /// @brief A "don't show this again" checkbox.
        LD_DONT_SHOW_AGAIN = 1 << 8,
        /// @brief A close button.
        LD_CLOSE_BUTTON = 1 << 9,
        /// @brief A sort button.
        LD_SORT_BUTTON = 1 << 10,
        /// @brief A clear button.
        LD_CLEAR_BUTTON = 1 << 11,
        /// @brief A refresh button.
        LD_REFRESH_BUTTON = 1 << 12,
        /// @brief A verbose logging toggle button.
        LD_LOG_VERBOSE_BUTTON = 1 << 13,
        /// @brief Make the list control single selection.
        LD_SINGLE_SELECTION = 1 << 14
        };

    /// @brief A dialog with a list control and various buttons.
    class ListDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param values The values to fill into the list.
            @param useCheckBoxes @c true to show checkboxes in the list control.
            @param parent The parent window.
            @param bkColor The dialog's background color.
            @param hoverColor The list control's hover color.
            @param foreColor The dialog's foreground color.
            @param buttonStyle The ListDlgFlags style for which features to include.
            @param id The dialog's ID.
            @param caption The dialog's caption.
            @param label The checkbox's label.
            @param pos The dialog's position.
            @param size The dialog's size.
            @param style The dialog's style.*/
        ListDlg(wxWindow* parent, const wxArrayString& values, const bool useCheckBoxes,
                const wxColour& bkColor, const wxColour& hoverColor, const wxColour& foreColor,
                const long buttonStyle = LD_NO_BUTTONS, wxWindowID id = wxID_ANY,
                const wxString& caption = wxString{}, wxString label = wxString{},
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize{ 600, 250 },
                long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        /** @brief Constructor that won't show the checkbox.
            @param parent The parent window.
            @param bkColor The dialog's background color.
            @param hoverColor The list control's hover color.
            @param foreColor The dialog's foreground color.
            @param buttonStyle The ListDlgFlags style for which features to include.
            @param id The dialog's ID.
            @param caption The dialog's caption.
            @param label The checkbox's label.
            @param pos The dialog's position.
            @param size The dialog's size.
            @param style The dialog's style.*/
        ListDlg(wxWindow* parent, const wxColour& bkColor, const wxColour& hoverColor,
                const wxColour& foreColor, const long buttonStyle = LD_NO_BUTTONS,
                wxWindowID id = wxID_ANY, const wxString& caption = wxString{},
                wxString label = wxString{}, const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxSize(600, 250),
                long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        /// @private
        ListDlg(const ListDlg&) = delete;
        /// @private
        ListDlg& operator=(const ListDlg&) = delete;

        /// @returns The list's data provider.
        [[nodiscard]]
        std::shared_ptr<ListCtrlExDataProvider>& GetData()
            {
            return m_data;
            }

        /// @returns The list.
        [[nodiscard]]
        ListCtrlEx* GetListCtrl() noexcept
            {
            return m_list;
            }

        /// @returns The dialog's ribbon.
        [[nodiscard]]
        wxRibbonBar* GetRibbon() noexcept
            {
            return m_ribbon;
            }

        /// @returns An array of the selected strings.
        [[nodiscard]]
        const wxArrayString& GetSelectedItems() const noexcept
            {
            return m_selectedItems;
            }

        /// @returns @c true if the checkbox at the bottom of the dialog is checked.
        [[nodiscard]]
        bool IsCheckBoxChecked() const noexcept
            {
            return m_dontShowAgain;
            }

        /// @brief Sets the label for the checkbox on the dialog.
        /// @param label The text to display.
        void SetCheckBoxLabel(const wxString& label)
            {
            if (m_checkBox != nullptr)
                {
                m_checkBox->SetLabel(label);
                }
            }

        /// @brief Sets the help sort topic for the list control.
        /// @param helpProjectPath The help folder.
        /// @param topicPath The subpath to the topic.
        void SetSortHelpTopic(const wxString& helpProjectPath, const wxString& topicPath)
            {
            if (m_list != nullptr)
                {
                m_list->SetSortHelpTopic(helpProjectPath, topicPath);
                }
            }

        /// @brief Sets the log file reporter to read and write from
        ///     (if this is meant to be a log report window).
        /// @param log The log file reporter to connect this dialog to.
        void SetActiveLog(LogFile* log);

        /// @brief If an active log is connected, reads its content into this dialog.
        /// @sa SetActiveLog().
        void ReadLog()
            {
            wxCommandEvent event;
            OnReadLog(event);
            }

        /// @brief If logging buttons are shown, toggles the verbose logging button.
        /// @param enable @c true to enable logging.
        void EnableVerboseLogging(const bool enable)
            {
            m_isLogVerbose = enable;
            if (((m_buttonStyle & LD_LOG_VERBOSE_BUTTON) != 0) && m_editButtonBar != nullptr)
                {
                m_editButtonBar->ToggleButton(XRCID("ID_VERBOSE_LOG"), m_isLogVerbose);
                }
            }

      private:
        void BindEvents();
        /// Creates the controls and sizers
        void CreateControls();
        void OnAffirmative(const wxCommandEvent& event);
        void OnNegative(const wxCommandEvent& event);
        void OnSelectAll([[maybe_unused]] wxRibbonButtonBarEvent& event);
        void OnCopy([[maybe_unused]] wxRibbonButtonBarEvent& event);
        void OnSave(wxRibbonButtonBarEvent& event);
        void OnPrint(wxRibbonButtonBarEvent& event);
        void OnSort(wxRibbonButtonBarEvent& event);
        void OnFind(wxFindDialogEvent& event);
        void OnClose([[maybe_unused]] wxCloseEvent& event);
        void OnReadLog([[maybe_unused]] wxCommandEvent& event);

        void StopRealtimeUpdate() { m_realTimeTimer.Stop(); }

        void RestartRealtimeUpdate()
            {
            if (m_logFile != nullptr && m_autoRefresh)
                {
                m_realTimeTimer.Start(REALTIME_UPDATE_INTERVAL);
                }
            }

        void OnRealTimeTimer([[maybe_unused]] wxTimerEvent& event);
        void OnRealTimeUpdate([[maybe_unused]] wxRibbonButtonBarEvent& event);

        bool m_useCheckBoxes{ true };
        long m_buttonStyle{ 0 };
        wxString m_label;
        wxColour m_hoverColor;
        bool m_dontShowAgain{ false };
        ListCtrlEx* m_list{ nullptr };
        wxCheckListBox* m_checkList{ nullptr };
        wxRibbonButtonBar* m_editButtonBar{ nullptr };
        std::shared_ptr<ListCtrlExDataProvider> m_data{
            std::make_shared<ListCtrlExDataProvider>()
        };
        LogFile* m_logFile{ nullptr };
        wxCheckBox* m_checkBox{ nullptr };
        wxRibbonBar* m_ribbon{ nullptr };
        wxArrayString m_values;
        wxArrayString m_selectedItems;

        constexpr static wxWindowID ID_EDIT_PANEL{ wxID_HIGHEST };
        constexpr static wxWindowID ID_EDIT_BUTTON_BAR{ wxID_HIGHEST + 1 };

        constexpr static int REALTIME_UPDATE_INTERVAL{ 3000 }; // in milliseconds
        wxTimer m_realTimeTimer;
        bool m_autoRefresh{ true };
        bool m_isLogVerbose{ false };
        wxDateTime m_sourceFileLastModified;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // LIST_DIALOG_H
