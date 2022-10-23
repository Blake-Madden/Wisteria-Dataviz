/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2022
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __INFOBAR_EX_H__
#define __INFOBAR_EX_H__

#include <wx/wx.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/valgen.h>
#include <wx/artprov.h>

namespace Wisteria::UI
    {
    /** @brief A rewrite of @c wxInfoBar with additional features.

        Includes the following changes:
        - Adds a "don't show this again" checkbox.
        - User clicking the Close button now sends the `wxID_CLOSE` event up to the parent so
            that it can handle it as well.
        - The icon is now 16x16, not 32x32.
        - Custom button support was removed to simplify things.

        To add additional behavior to execute after the info bar is closed, connect it to your
        own button handler using the `wxID_CLOSE` ID.

    @code
        [the info bar control]->Connect(wxID_CLOSE, wxEVT_BUTTON,
                                        wxCommandEventHandler([button handler function]),
                                        nullptr,
                                        this);
    @endcode
    */
    class InfoBarEx : public wxControl
        {
    public:
        /// @brief The default CTOR that should be used with Create()
        ///     (but remember that info bar is created hidden).
        InfoBarEx() = default;
        /// @private
        InfoBarEx(const InfoBarEx&) = delete;
        /// @private
        InfoBarEx(InfoBarEx&&) = delete;
        /// @private
        InfoBarEx& operator=(const InfoBarEx&) = delete;
        /// @private
        InfoBarEx& operator=(InfoBarEx&&) = delete;

        /** @brief Constructor.
            @param parent The parent window.
            @param winid The window ID.
            @param includeDontShowAgain Whether to show a "don't show this again" checkbox.*/
        InfoBarEx(wxWindow *parent, wxWindowID winid = wxID_ANY,
            const bool includeDontShowAgain = true) :
            m_includeDontShowAgain(includeDontShowAgain)
            { Create(parent, winid); }

        /** @brief Creates the control.
            @param parent The control's parent.
            @param winid The control's ID.
            @returns @c true upon success.*/
        bool Create(wxWindow* parent, wxWindowID winid = wxID_ANY);

        /** @brief Shows a message.
            @param msg The message to display.
            @param flags The flags to use.*/
        void ShowMessage(const wxString& msg,
                         int flags = wxICON_INFORMATION);

        /// @returns Whether the "Do not show this again" checkbox was checked at the
        ///     time of the window being closed.
        /// @note This should be called in your handler for the `wxID_CLOSE button` being clicked.
        bool IsDontShowAgainChecked() const
            { return m_dontShowAgain; }

        /// @returns Whether the "Do not show this again" checkbox is being shown.
        bool IsIncludingDontShowAgainCheckbox() const
            { return m_includeDontShowAgain; }
        /// @brief Sets whether the "Do not show this again" checkbox should be shown.
        /// @param includeCheckbox Whether to include the checkbox.
        void IncludeDontShowAgainCheckbox(const bool includeCheckbox)
            {
            m_includeDontShowAgain = includeCheckbox;
            GetSizer()->Show(m_dontShowAgainCheckbox, m_includeDontShowAgain, true);
            }

        /// @brief Dismisses the control.
        void Dismiss();

        // methods specific to this version
        // --------------------------------

        /** @brief Set the effect(s) to use when showing/hiding the bar.
            @details may be @c wxSHOW_EFFECT_NONE to disable any effects entirely
                by default, slide to bottom/top is used when it's positioned on the top
                of the window for showing/hiding it and top/bottom when it's positioned
                at the bottom.
            @param showEffect The effect to use when showing the control.
            @param hideEffect The effect to use when hiding the control.*/
        void SetShowHideEffects(wxShowEffect showEffect, wxShowEffect hideEffect)
            {
            m_showEffect = showEffect;
            m_hideEffect = hideEffect;
            }

        /// @returns The effect used when showing the window.
        wxShowEffect GetShowEffect() const;
        /// @returns The effect used when hiding the window.
        wxShowEffect GetHideEffect() const;

        /// @brief Set the duration of animation used when showing/hiding the bar, in ms.
        /// @param duration The duration time.
        void SetEffectDuration(int duration) { m_effectDuration = duration; }

        /// @returns The currently used effect animation duration.
        int GetEffectDuration() const { return m_effectDuration; }

        // overridden base class methods
        // -----------------------------

        /// @brief Same thing with the color: this affects the text color.
        /// @param colour The color to use.
        /// @returns @c true upon success.
        bool SetForegroundColour(const wxColour& colour) final;
    protected:
        /// @returns @c wxBORDER_NONE.
        /// @brief Info bar shouldn't have any border by default, the color difference
        ///     between it and the main window separates it well enough.
        wxBorder GetDefaultBorder() const final { return wxBORDER_NONE; }

        /// @brief Updates the parent to take our new or changed size into account (notably
        ///     should be called when we're shown or hidden)
        void UpdateParent();
    private:
        // handler for the close button
        void OnButton(wxCommandEvent& event);

        // show/hide the bar
        void DoShow();
        void DoHide();

        // determine the placement of the bar from its position in the containing sizer
        enum class BarPlacement
            {
            BarPlacement_Top,
            BarPlacement_Bottom,
            BarPlacement_Unknown
            };

        BarPlacement GetBarPlacement() const;

        // different controls making up the bar
        wxStaticBitmap* m_icon{ nullptr };
        wxStaticText* m_text{ nullptr };
        wxCheckBox* m_dontShowAgainCheckbox{ nullptr };

        // the effects to use when showing/hiding and duration for them: by default
        // the effect is determined by the info bar automatically depending on its
        // position and the default duration is used
        wxShowEffect m_showEffect{ wxSHOW_EFFECT_MAX },
                     m_hideEffect{ wxSHOW_EFFECT_MAX };
        int m_effectDuration{ 0 };

        bool m_includeDontShowAgain{ true };
        bool m_dontShowAgain{ false };
        };
    }

/** @}*/

#endif // __INFOBAR_EX_H__

