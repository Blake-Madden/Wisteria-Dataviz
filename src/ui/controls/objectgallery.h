/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_OBJECT_GALLERY_H
#define WISTERIA_OBJECT_GALLERY_H

#include "../../base/enums.h"
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <wx/timer.h>
#include <wx/webview.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    class ObjectGalleryItemDroppedEvent;

    /// @cond DOXYGEN_IGNORE
    wxDECLARE_EVENT(wxEVT_OBJECTGALLERY_ITEM_DROPPED, ObjectGalleryItemDroppedEvent);

    /// @endcond

    /// @brief Event fired (on the gallery's configured drop target window, not on the
    ///     gallery itself) when a tile is dragged out of the object gallery and released.
    class ObjectGalleryItemDroppedEvent final : public wxCommandEvent
        {
      public:
        /// @private
        ObjectGalleryItemDroppedEvent() = default;

        /** @brief Constructor.
            @param commandType The event type.
            @param winId The ID of the window the event is being sent to.
            @param itemType The type of object that was dropped.
            @param screenPos The screen position where the drag ended.*/
        ObjectGalleryItemDroppedEvent(const wxEventType commandType, const wxWindowID winId,
                                      const GalleryItemType itemType, const wxPoint& screenPos)
            : wxCommandEvent(commandType, winId), m_itemType(itemType), m_screenPos(screenPos)
            {
            }

        /// @returns The type of object that was dropped.
        [[nodiscard]]
        GalleryItemType GetItemType() const noexcept
            {
            return m_itemType;
            }

        /// @returns The screen position where the drag ended.
        [[nodiscard]]
        wxPoint GetDropScreenPosition() const noexcept
            {
            return m_screenPos;
            }

        /// @private
        [[nodiscard]]
        wxEvent* Clone() const override
            {
            return new ObjectGalleryItemDroppedEvent(*this);
            }

      private:
        GalleryItemType m_itemType{ GalleryItemType::Label };
        wxPoint m_screenPos;
        };

    /** @brief Grouped gallery of every object type that can be placed
            onto a page's grid, rendered with a wxWebView.
        @details Dragging a tile onto another window fires @c wxEVT_OBJECTGALLERY_ITEM_DROPPED
            (an ObjectGalleryItemDroppedEvent) on that window (not on the gallery itself).\n
            Dragging is simulated (a floating icon follows the cursor, tracked by polling
            the mouse position) rather than using OS-level drag-and-drop, since content
            inside a wxWebView cannot participate in native drag sources.*/
    class ObjectGalleryCtrl final : public wxPanel
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param dropTarget The window that dropped items should be reported against
                (i.e., where @c wxEVT_OBJECTGALLERY_ITEM_DROPPED will be sent).
                Typically the page's grid preview panel.
            @param id The window ID.
            @param pos The position.
            @param size The size.*/
        explicit ObjectGalleryCtrl(wxWindow* parent, wxWindow* dropTarget, wxWindowID id = wxID_ANY,
                                   const wxPoint& pos = wxDefaultPosition,
                                   const wxSize& size = wxDefaultSize);

        /// @private
        ObjectGalleryCtrl() = delete;
        /// @private
        ObjectGalleryCtrl(const ObjectGalleryCtrl&) = delete;
        /// @private
        ObjectGalleryCtrl& operator=(const ObjectGalleryCtrl&) = delete;

        /** @brief Enables or disables a tile in the gallery.
            @param itemType The item to enable/disable.
            @param enable @c true to enable the tile (the default state).
            @param disabledTooltip The tooltip to show on the tile while disabled.*/
        void EnableItem(GalleryItemType itemType, bool enable,
                        const wxString& disabledTooltip = wxString{});

      private:
        [[nodiscard]]
        wxString BuildHtml() const;
        [[nodiscard]]
        static wxString GetGroupDisplayName(GalleryGroup group);
        [[nodiscard]]
        static wxString GetGroupIconName(GalleryGroup group);
        [[nodiscard]]
        static wxString GalleryItemTypeToId(GalleryItemType type);
        [[nodiscard]]
        static std::optional<GalleryItemType> IdToGalleryItemType(const wxString& id);

        void RefreshPage();
        void OnSysColourChanged(wxSysColourChangedEvent& event);
        void OnScriptMessage(wxWebViewEvent& event);
        void OnDragTimer(wxTimerEvent& event);
        void StartDrag(GalleryItemType itemType);
        void EndDrag(bool viaMessage);
        void PersistCollapsedGroups() const;

        constexpr static std::wstring_view ScriptMessageHandlerName{ L"wisteriaGallery" };

        wxWebView* m_webView{ nullptr };
        wxWindow* m_dropTarget{ nullptr };
        wxFrame* m_ghostWindow{ nullptr };
        wxStaticBitmap* m_ghostBitmapCtrl{ nullptr };
        wxTimer m_dragTimer;
        bool m_isDragging{ false };
        GalleryItemType m_draggedItemType{ GalleryItemType::Label };
        std::map<GalleryItemType, wxString> m_disabledItemTooltips;
        // groups the user has collapsed, remembered here so a rebuild of the
        // page (e.g., on a theme change) keeps them collapsed
        std::set<GalleryGroup> m_collapsedGroups;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_OBJECT_GALLERY_H
