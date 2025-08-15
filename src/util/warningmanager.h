/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WARNING_MANAGER_H__
#define __WARNING_MANAGER_H__

#include <algorithm>
#include <vector>
#include <wx/wx.h>

/// @brief An enhanced warning message that can store user response information.
struct WarningMessage
    {
    /// @brief Constructor that is only used for quick searching when all you have is the ID.
    /// @param id The messages ID.
    explicit WarningMessage(wxString id) : m_Id(std::move(id)) {}

    /// @brief Constructor.
    /// @param id The messages ID.
    /// @param message The message.
    /// @param title The title.
    /// @param description The description.
    /// @param flags The flags.
    /// @param onlyShowOnce @c true if the warning should only be shown once to the user.
    WarningMessage(wxString id, wxString message, wxString title, wxString description,
                   const int flags, const bool onlyShowOnce = false)
        : m_Id(std::move(id)), m_Message(std::move(message)), m_title(std::move(title)),
          m_description(std::move(description)), m_flags(flags), m_onlyShowOnce(onlyShowOnce)
        {
        }

    /// @private
    [[nodiscard]]
    bool operator<(const WarningMessage& that) const
        {
        return m_Id < that.m_Id;
        }

    /// @private
    [[nodiscard]]
    bool operator==(const WarningMessage& that) const
        {
        return m_Id == that.m_Id;
        }

    /// @private
    [[nodiscard]]
    bool operator==(const wxString& id) const
        {
        return m_Id == id;
        }

    /// @returns The ID.
    [[nodiscard]]
    const wxString& GetId() const noexcept
        {
        return m_Id;
        }

    /// @returns The message.
    [[nodiscard]]
    const wxString& GetMessage() const noexcept
        {
        return m_Message;
        }

    /// @brief Sets the message.
    /// @param message The message to use.
    void SetMessage(const wxString& message) { m_Message = message; }

    /// @returns The title.
    [[nodiscard]]
    const wxString& GetTitle() const noexcept
        {
        return m_title;
        }

    /// @returns The description.
    [[nodiscard]]
    const wxString& GetDescription() const noexcept
        {
        return m_description;
        }

    /// @returns The flags.
    [[nodiscard]]
    int GetFlags() const noexcept
        {
        return m_flags;
        }

    /// @brief Whether the message should be shown.
    /// @param show @c true if it should be shown to the user.
    void Show(const bool show) noexcept { m_showWarning = show; }

    /// @private
    [[nodiscard]]
    bool& ShouldBeShown() noexcept
        {
        return m_showWarning;
        }

    /// @returns @c true if the warning should be shown.
    [[nodiscard]]
    bool ShouldBeShown() const noexcept
        {
        return m_showWarning;
        }

    /// @returns @c true if the warning should only be shown once to the user.
    [[nodiscard]]
    bool ShouldOnlyBeShownOnce() const noexcept
        {
        return m_onlyShowOnce;
        }

    /// @returns The previous response.
    [[nodiscard]]
    int GetPreviousResponse() const noexcept
        {
        return m_previousResponse;
        }

    /// @brief Sets the response from the user.
    /// @param response The response.
    void SetPreviousResponse(const int response) noexcept { m_previousResponse = response; }

  private:
    wxString m_Id;
    wxString m_Message;
    wxString m_title;
    wxString m_description;
    int m_flags{ 0 };
    bool m_showWarning{ true };
    bool m_onlyShowOnce{ false };
    int m_previousResponse{ 0 };
    };

/// @brief Management system for warning messages to be shown to the user.
/// @details This class is entirely static and should be used as a pseudo-singleton.
///     Do not create individual instances.
class WarningManager
    {
  public:
    /// @returns The vector of warning messages.
    [[nodiscard]]
    static std::vector<WarningMessage>& GetWarnings() noexcept
        {
        return m_warningManager;
        }

    /// @brief Adds a warning to the system.
    /// @param message The warning message to add to the system.
    static void AddWarning(const WarningMessage& message)
        {
        m_warningManager.insert(
            (std::lower_bound(m_warningManager.begin(), m_warningManager.end(), message)), message);
        }

    /// @brief Enables all the warnings to be shown and not have a valid previous response.
    static void EnableWarnings() noexcept
        {
        for (auto& warning : m_warningManager)
            {
            warning.Show(true);
            warning.SetPreviousResponse(0);
            }
        }

    /// @brief Enables the specified warning to be shown and not have a valid previous response.
    /// @param messageId The message ID to search for.
    static void EnableWarning(const wxString& messageId) noexcept
        {
        const auto warning = GetWarning(messageId);
        if (warning != m_warningManager.end())
            {
            warning->Show(true);
            warning->SetPreviousResponse(0);
            }
        }

    /// @brief Disables the specified warning to be shown and have an
    ///     affirmative previous response.
    /// @param messageId The message ID to search for.
    static void DisableWarning(const wxString& messageId) noexcept
        {
        const auto warning = GetWarning(messageId);
        if (warning != m_warningManager.end())
            {
            warning->Show(false);
            warning->SetPreviousResponse(wxID_YES);
            }
        }

    /// @brief Suppresses all warnings from being shown.
    static void DisableWarnings() noexcept
        {
        for (auto& warning : m_warningManager)
            {
            warning.Show(false);
            warning.SetPreviousResponse(wxID_YES);
            }
        }

    /// @returns A warning iterator by the given ID. Returns an invalid iterator if not found.
    /// @param messageId The ID of the warning to access.
    [[nodiscard]]
    static std::vector<WarningMessage>::iterator GetWarning(const wxString& messageId)
        {
        std::vector<WarningMessage>::iterator warningPos = std::lower_bound(
            m_warningManager.begin(), m_warningManager.end(), WarningMessage(messageId));
        return (warningPos != m_warningManager.end() && warningPos->GetId() == messageId) ?
                   warningPos :
                   m_warningManager.end();
        }

    /// @returns Whether a warning message is (by ID) is in the system.
    /// @param messageId The ID of the warning to find.
    [[nodiscard]]
    static bool HasWarning(const wxString& messageId)
        {
        std::vector<WarningMessage>::iterator warningPos = std::lower_bound(
            m_warningManager.begin(), m_warningManager.end(), WarningMessage(messageId));
        return (warningPos != m_warningManager.end() && warningPos->GetId() == messageId);
        }

  private:
    static std::vector<WarningMessage> m_warningManager;
    };

    /** @}*/

#endif //__WARNING_MANAGER_H__
