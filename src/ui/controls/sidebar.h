/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SIDEBAR_H
#define WISTERIA_SIDEBAR_H

#include <algorithm>
#include <optional>
#include <set>
#include <vector>
#include <wx/scrolwin.h>
#include <wx/uilocale.h>
#include <wx/window.h>
#include <wx/wx.h>

/// @cond DOXYGEN_IGNORE
wxDECLARE_EVENT(wxEVT_SIDEBAR_CLICK, wxCommandEvent);

#define EVT_SIDEBAR_CLICK(winid, fn)                                                               \
    wx__DECLARE_EVT1(wxEVT_SIDEBAR_CLICK, winid, wxCommandEventHandler(fn))

/// @endcond

namespace Wisteria::UI
    {
    /// @brief Visual effect for a sidebar control.
    enum class SidebarStyle
        {
        /// @brief A glassy appearance
        Glassy,
        /// @brief A flat appearance.
        Flat
        };

    /// @brief A scheme of colors to apply to a sidebar control.
    struct SideBarColorScheme
        {
        /// @brief Background color of control.
        wxColour m_backgroundColor;
        /// @brief Text color.
        wxColour m_foregroundColor;
        /// @brief Background color of item when it's selected.
        wxColour m_selectedColor;
        /// @brief Font color of item when it's selected.
        wxColour m_selectedFontColor;
        /// @brief Background color of folders.
        wxColour m_parentColor;
        /// @brief Background color of item when mouse is over it.
        wxColour m_highlightColor;
        /// @brief Font color of item when mouse is over it.
        wxColour m_highlightFontColor;
        };

    /** @brief A visually-enhanced tree control, similar to Outlook's sidebar.
        @details The control organizes data into a folder
            and items (sub-items under the folders).*/
    class SideBar final : public wxScrolledCanvas
        {
      public:
        /// @brief Constructor.
        /// @param parent The parent window.
        /// @param id The control's ID.
        explicit SideBar(wxWindow* parent, wxWindowID id = wxID_ANY);
        /// @private
        SideBar(const SideBar&) = delete;
        /// @private
        SideBar& operator=(const SideBar&) = delete;

        /// @brief A child item (i.e., child of a "folder") in the sidebar.
        struct SideBarSubItem
            {
            /// @private
            [[nodiscard]]
            bool operator<(const SideBarSubItem& that) const
                {
                return wxUILocale::GetCurrent().CompareStrings(m_label, that.m_label,
                                                               wxCompare_CaseInsensitive) < 0;
                }

            /// @brief The string to display for the item.
            wxString m_label;
            /// @brief An ID to assign to the item.
            wxWindowID m_id{ wxID_ANY };
            /// @brief Index into the image list to use for the item.
            /// @details Leave as @c std::nullopt to not show an icon.
            std::optional<size_t> m_iconIndex{ std::nullopt };
            /// @private
            wxRect m_Rect;
            };

        /// @brief A top-level (i.e., "folder") item in the sidebar.
        class SideBarItem
            {
            friend class SideBar;

          public:
            /// @brief Opens the folder to display its children nodes.
            void Expand() noexcept
                {
                if (!m_subItems.empty())
                    {
                    m_isExpanded = true;
                    }
                }

            /// @brief Closes the folder, hiding its children.
            void Collapse() noexcept
                {
                if (!m_subItems.empty())
                    {
                    m_isExpanded = false;
                    }
                }

            /// @returns The number of child items in this folder.
            [[nodiscard]]
            size_t GetSubItemCount() const noexcept
                {
                return m_subItems.size();
                }

            /// @returns @c true if the selected item in the sidebar is
            ///     one of this folder's children.
            /// @warning Folders store the selected subitem even when it isn't active.
            ///     To see if a folder is the currently selected one, use IsActive().
            [[nodiscard]]
            bool IsSubItemSelected() const
                {
                return (m_selectedItem.has_value() && m_selectedItem.value() < m_subItems.size());
                }

            /// @returns @c true if this is the active folder (i.e., most recently selected one).
            [[nodiscard]]
            bool IsActive() const noexcept
                {
                return m_isActive;
                }

            /// @brief Sorts the folder's subitems alphabetically
            ///     (locale sensitive, case insensitively).
            void SortSubItems() { std::sort(m_subItems.begin(), m_subItems.end()); }

            /// @returns The item's ID.
            [[nodiscard]]
            wxWindowID GetId() const noexcept
                {
                return m_id;
                }

          private:
            wxString m_label;
            wxWindowID m_id{ wxID_ANY };
            std::optional<size_t> m_iconIndex{ std::nullopt };
            wxRect m_Rect;
            std::vector<SideBarSubItem> m_subItems;
            std::optional<size_t> m_highlightedItem{ std::nullopt };
            std::optional<size_t> m_selectedItem{ std::nullopt };
            bool m_isExpanded{ false };
            bool m_isActive{ false };
            };

        /** @brief Collapses all the items that have subitems.*/
        void CollapseAll();
        /** @brief Expands all the items that have subitems.*/
        void ExpandAll();

        /// @name Editing Functions
        /// @brief Functions relating to inserting and editing items.
        /// @{

        /** @brief Inserts a (root level) item.
            @param position The positions to insert the item.
            @param label The label to be displayed on the item.
            @param Id The numeric ID of the item.
            @param iconIndex The icon (index into the image list) to be displayed on the item.\n
                Set to @c std::optional to not display an icon.*/
        void InsertItem(const size_t position, const wxString& label, const wxWindowID Id,
                        std::optional<size_t> iconIndex);
        /** @brief Insert a subitem under the specified root item.
            @param parentItemId The ID of the parent item to insert this under.
            @param label The label to display on the item.
            @param Id The numeric ID of the item.
            @param iconIndex The icon (index into the image list) to be displayed on the item.\n
                Set to @c std::optional to not display an icon.
            @returns @c true if the item was inserted, @c false if not added
                (e.g., will fail if parent item is not found).*/
        bool InsertSubItemById(const wxWindowID parentItemId, const wxString& label,
                               const wxWindowID Id, std::optional<size_t> iconIndex);

        /** @brief Accesses a (root-level) item.
            @param item The item to access (by index).
            @returns The folder at item index.*/
        [[nodiscard]]
        SideBarItem& GetFolder(const size_t item)
            {
            assert(item < m_folders.size() && L"Invalid item in call to GetFolder()!");
            return m_folders.at(item);
            }

        /** @brief Deletes all items from the sidebar.*/
        void DeleteAllFolders()
            {
            m_folders.clear();
            m_highlightedFolder.reset();
            m_selectedFolder.reset();
            RecalcSizes();
            Refresh();
            }

        /** @brief Deletes a specific (root-level) folder (by index).
            @param index The index of the folder to delete.*/
        void DeleteFolder(const size_t index)
            {
            m_folders.erase(m_folders.begin() + index);
            m_highlightedFolder.reset();
            if (m_folders.empty())
                {
                m_selectedFolder.reset();
                }
            else if (!m_selectedFolder.has_value() || m_selectedFolder.value() >= m_folders.size())
                {
                m_selectedFolder = 0;
                }
            RecalcSizes();
            Refresh();
            }

        /** @brief Sets the control's image list.
            @param imageList A vector of bitmaps to use as the image list.*/
        void SetImageList(const std::vector<wxBitmapBundle>& imageList) { m_imageList = imageList; }

        /** @returns The control's image list.*/
        [[nodiscard]]
        std::vector<wxBitmapBundle>& GetImageList() noexcept
            {
            return m_imageList;
            }

        /// @}

        /// @name Folder Functions
        /// @brief Functions relating to root-level folder (not related to editing).
        /// @{

        /** @returns The total of (root level) items.
            @note Subitems are not included in this count.*/
        [[nodiscard]]
        size_t GetFolderCount() const noexcept
            {
            return m_folders.size();
            }

        /** @returns The label of a given root item.
            @param item The index of the item in the list of root items.*/
        [[nodiscard]]
        wxString GetFolderText(const size_t item) const
            {
            if (item >= GetFolderCount())
                {
                return wxEmptyString;
                }
            return m_folders[item].m_label;
            }

        /** @brief Scrolls to folder (by index) if not fully visible.
            @param index The index of the folder to make visible.*/
        void EnsureFolderVisible(const size_t index);
        /// @}

        /// @name Selection Functions
        /// @brief Functions relating to item selection.
        /// @{

        /** @brief Selects a root-level item.
            @param item The position of the item to select.
            @param setFocus Whether to set the keyboard focus to the control.
            @param sendEvent Whether to send an @c EVT_SIDEBAR_CLICK event.
            @param collapseIfExpanded @c true to collapse the folder if already expanded.\n
                If collapsing the folder, then @c sendEvent will be overridden
                (i.e., the selection event will not be fired).
            @returns @c true if the folder was selected.*/
        bool SelectFolder(const size_t item, const bool setFocus = true,
                          const bool sendEvent = true, const bool collapseIfExpanded = false);
        /** @brief Selects a subitem.
            @param item The position of the parent item to select.
            @param subItem The position of the subitem (relative to its parent) to select.
            @param setFocus Whether to set the keyboard focus to the control.
            @param sendEvent Whether to send a @c EVT_SIDEBAR_CLICK event.
            @returns @c true if the subitem (or at least the folder) was found and selected.*/
        bool SelectSubItem(const size_t item, const size_t subItem, const bool setFocus = true,
                           const bool sendEvent = true);

        /** @brief Selects a subitem.
            @param item The position of the parent item and subitem (relative to its parent) to
           select.
            @param setFocus Whether to set the keyboard focus to the control.
            @param sendEvent Whether to send a @c EVT_SIDEBAR_CLICK event.
            @returns @c true if the subitem was found and selected.*/
        bool SelectSubItem(const std::pair<size_t, size_t>& item, const bool setFocus = true,
                           const bool sendEvent = true)
            {
            return SelectSubItem(item.first, item.second, setFocus, sendEvent);
            }

        /** @brief Selects a folder or subitem with the provided ID.
            @param folderId The ID of the parent item to select.
            @param subItemId The ID of the item to select.
            @param setFocus Whether to set the keyboard focus to the control.
            @param sendEvent Whether to send a @c EVT_SIDEBAR_CLICK event.
            @returns @c true if the item was found and selected.*/
        bool SelectSubItemById(const wxWindowID folderId, const wxWindowID subItemId,
                               const bool setFocus = true, const bool sendEvent = true);

        /** @brief Selects a folder or subitem with the provided ID.
            @param folderAndSubItemId The position of the parent item and ID of the item to select.
            @param setFocus Whether to set the keyboard focus to the control.
            @param sendEvent Whether to send a @c EVT_SIDEBAR_CLICK event.
            @returns @c true if the item was found and selected.*/
        bool SelectSubItemById(const std::pair<size_t, wxWindowID>& folderAndSubItemId,
                               const bool setFocus = true, const bool sendEvent = true)
            {
            return SelectSubItemById(folderAndSubItemId.first, folderAndSubItemId.second, setFocus,
                                     sendEvent);
            }

        /** @brief Selects a folder or subitem.
            @param item The position of the item to select.
            @param setFocus Whether to set the keyboard focus to the control.
            @param sendEvent Whether to send a @c EVT_SIDEBAR_CLICK event.*/
        void SelectAnyItem(const size_t item, const bool setFocus = true,
                           const bool sendEvent = true);

        /** @returns The position of the selected root-level item, or @c std::nullopt
                if nothing is selected.
            @note If a subitem is selected, this will return the position of its parent item.*/
        [[nodiscard]]
        std::optional<size_t> GetSelectedFolder() const noexcept
            {
            return m_selectedFolder;
            }

        /** @brief Gets the position of the selected item (or sub item).
            @returns The raw position of the selected item,
                meaning the index of the item including subitems.\n
                Returns @c std::nullopt if nothing is selected.*/
        [[nodiscard]]
        std::optional<size_t> GetSelectedAnyItem() const;

        /** @returns The ID of the selected root-level item,
                or @c std::nullopt if nothing is selected.
            @note If a subitem is selected, this will return the ID of its parent item.*/
        [[nodiscard]]
        std::optional<wxWindowID> GetSelectedFolderId() const
            {
            return IsFolderSelected() ?
                       std::optional<wxWindowID>(m_folders[GetSelectedFolder().value()].m_id) :
                       std::nullopt;
            }

        /** @brief Gets parent ID and ID of its selected subitem, or a pair of
                @c std::nullopt if no subitem is selected.
            @returns The position of the parent and the subitem's ID.*/
        [[nodiscard]]
        std::pair<std::optional<wxWindowID>, std::optional<wxWindowID>>
        GetSelectedSubItemId() const;
        /** @returns The label of the selected folder
                (or subitem if the folder has a select subitem).*/
        [[nodiscard]]
        wxString GetSelectedLabel() const;

        /** @returns Whether a (root-level) item is selected in the sidebar.*/
        [[nodiscard]]
        bool IsFolderSelected() const
            {
            return (m_selectedFolder.has_value() && m_selectedFolder.value() < GetFolderCount());
            }

        /// @}

        /// @name Search Functions
        /// @brief Functions relating to searching for items in the control.
        /// @{

        /** @brief Search for a (root-level) item by ID.
            @param Id The ID of the item to search for.
            @return The position of the item, or @c std::nullopt if not found.*/
        [[nodiscard]]
        std::optional<size_t> FindFolder(const wxWindowID Id) const;
        /** @brief Searches for a sub item by ID.
            @note This it will search within all the root-level items.
            @param Id The ID of the sub item to search for.
            @returns If found, returns the parent's index and the subitem's index. Returns a
                pair of @c std::nullopt otherwise.*/
        [[nodiscard]]
        std::pair<std::optional<size_t>, std::optional<size_t>>
        FindSubItem(const wxWindowID Id) const;
        /** @brief Searches for a sub item by ID, but only within a specified parent item
            @param parentId The parent item's ID to search within.
            @param subItemId The ID of the sub item to search for.
            @returns If found, returns the parent's index and the subitem's index. Returns a
                pair of @c std::nullopt otherwise.*/
        [[nodiscard]]
        std::pair<std::optional<size_t>, std::optional<size_t>>
        FindSubItem(const wxWindowID parentId, const wxWindowID subItemId) const;
        /** Searches for a sub item by ID, but only within a specified parent item
            @param parentId The parent item's ID to search within.
            @param subItem Information about the subitem to search for. While try to find
                the best match based on @c subItem's ID, label, and icon.
            @returns If found, returns the parent's index and the subitem's index. Returns a
                pair of @c std::nullopt otherwise.*/
        [[nodiscard]]
        std::pair<std::optional<size_t>, std::optional<size_t>>
        FindSubItem(const wxWindowID parentId, const SideBarSubItem& subItem) const;
        /** @brief Searches for a subitem by label
            @details Note that it will search within all the root-level items.
            @param label The Label of the sub item to search for.
            @returns If found, returns the parent's index and the subitem's index. Returns a
                pair of @c std::nullopt otherwise.*/
        [[nodiscard]]
        std::pair<std::optional<size_t>, std::optional<size_t>>
        FindSubItem(const wxString& label) const;

        /// @}

        /// @name Appearance Functions
        /// @brief Functions relating to the appearance of the control.
        /// @{

        /** @brief Sets the color for the currently selected item.
            @param color The color to use.*/
        void SetSelectedColour(const wxColour& color) noexcept { m_selectedColor = color; }

        /** @brief Sets the font color for the currently selected item.
            @param color The color to use.*/
        void SetSelectedFontColour(const wxColour& color) noexcept { m_selectedFontColor = color; }

        /** @brief Sets the color for the parents.
            @param color The color to use.*/
        void SetParentColour(const wxColour& color) noexcept { m_parentColor = color; }

        /** @brief Sets the color for items that are being moused over.
            @param color The color to use.*/
        void SetHighlightColour(const wxColour& color) noexcept { m_highlightColor = color; }

        /** @brief Sets the font color for items that are being moused over.
            @param color The color to use.*/
        void SetHighlightFontColour(const wxColour& color) noexcept
            {
            m_highlightFontColor = color;
            }

        /** @brief Sets the entire coloring schema of the control.
            @param colorScheme The color scheme to use.*/
        void SetColorScheme(const SideBarColorScheme& colorScheme)
            {
            SetBackgroundColour(colorScheme.m_backgroundColor);
            SetForegroundColour(colorScheme.m_foregroundColor);
            SetSelectedColour(colorScheme.m_selectedColor);
            SetSelectedFontColour(colorScheme.m_selectedFontColor);
            SetParentColour(colorScheme.m_parentColor);
            SetHighlightColour(colorScheme.m_highlightColor);
            SetHighlightFontColour(colorScheme.m_highlightFontColor);
            }

        /// @returns Which sort of effect is being used to render the control.
        [[nodiscard]]
        SidebarStyle GetStyle() const noexcept
            {
            return m_style;
            }

        /// @brief Sets the visual effect to render the control.
        /// @param style The style to use.
        void SetStyle(const SidebarStyle style) noexcept { m_style = style; }

        /** @brief Sets the minimum width of the control to fit its widest item.
            @returns The new width of the control.*/
        size_t AdjustWidthToFitItems();
        /// @}

        /// @name Show/Hide Functions
        /// @brief Functions relating to minimizing and maximizing the control.
        /// @{

        /// @brief Shows the control, where the width is set to fit the content.
        void Maximize();
        /// @brief Hides the control, where the width is zero.
        void Minimize();
        /// @}

        /// @brief Saves information about which items are selected and expanded.
        void SaveState();
        /// @brief Selects and expands/collapses items previously saved from call to SaveState().
        void ResetState();

        /// @brief Perform initial layout and size calculations.
        /// @note This should be called after filling the image list and adding all items.
        void Realize();

        /** @brief Sets the size for all the icons (in DIPs).
            @param sz The icons' sizes.*/
        void SetIconSize(const wxSize sz) { m_iconSizeDIPs = sz; }

        /// @private
        [[nodiscard]]
        const std::vector<wxBitmapBundle>& GetImageList() const noexcept
            {
            return m_imageList;
            }

        /// @private
        [[nodiscard]]
        const SideBarItem& GetFolder(const size_t item) const
            {
            assert(item < m_folders.size() && L"Invalid item in call to GetFolder()!");
            return m_folders.at(item);
            }

        /// @private
        [[nodiscard]]
        std::optional<size_t> FindFolder(const std::optional<wxWindowID> Id) const
            {
            return (Id.has_value() ? FindFolder(Id.value()) : std::nullopt);
            }

        /// @private
        [[nodiscard]]
        std::pair<std::optional<size_t>, std::optional<size_t>>
        FindSubItem(std::optional<wxWindowID> parentId, std::optional<wxWindowID> subItemId) const
            {
            return ((parentId.has_value() && subItemId.has_value()) ?
                        FindSubItem(parentId.value(), subItemId.value()) :
                        std::make_pair(std::nullopt, std::nullopt));
            }

        /// @private
        [[nodiscard]]
        std::pair<std::optional<size_t>, std::optional<size_t>>
        FindSubItem(std::optional<wxWindowID> Id) const
            {
            return (Id.has_value() ? FindSubItem(Id.value()) :
                                     std::make_pair(std::nullopt, std::nullopt));
            }

        /// @private
        void SelectFolder(std::optional<size_t> item, const bool setFocus = true,
                          const bool sendEvent = true)
            {
            if (item.has_value())
                {
                SelectFolder(item.value(), setFocus, sendEvent);
                }
            }

        /// @private
        bool SelectSubItem(const std::pair<std::optional<size_t>, std::optional<size_t>>& item,
                           const bool setFocus = true, const bool sendEvent = true)
            {
            if (item.first.has_value() && item.second.has_value())
                {
                return SelectSubItem(item.first.value(), item.second.value(), setFocus, sendEvent);
                }
            return false;
            }

        /// @private
        bool SelectSubItemById(const std::pair<std::optional<wxWindowID>,
                                               std::optional<wxWindowID>>& folderAndSubItemId,
                               const bool setFocus = true, const bool sendEvent = true)
            {
            if (folderAndSubItemId.first.has_value() && folderAndSubItemId.second.has_value())
                {
                return SelectSubItemById(folderAndSubItemId.first.value(),
                                         folderAndSubItemId.second.value(), setFocus, sendEvent);
                }
            return false;
            }

        /// @brief Updates the areas and positions for the items and returns
        ///     their collective height.
        /// @returns The total height of all items and the height to where the active item is.
        /// @internal This is only useful for clients if trying to measure the height of the
        ///     content (for something like screenshots).
        /// @private
        std::pair<size_t, size_t> CalculateItemRects();

      private:
        void ActivateFolder(const size_t item)
            {
            for (auto& folder : m_folders)
                {
                folder.m_isActive = false;
                }
            if (item < GetFolderCount())
                {
                m_folders[item].m_isActive = true;
                }
            }

        /** @brief Gets the width (label, icon, and padding) of a given root item.
            @details The item's subitem width are factored into this (including their margins),
                so the width of the widest subitem will be returned if wider than the root item.
            @param item The position of the root item.
            @returns The item's width.*/
        [[nodiscard]]
        size_t GetFolderWidth(const size_t item);
        /// @brief Recalculates the control's content.
        void RecalcSizes();
        /// @brief Removes selection.
        void ClearHighlightedItems() noexcept;

        /// @returns @c true if an item's icon ID is valid.
        /// @param iconId The icon ID to validate.
        bool IsValidImageId(std::optional<size_t> iconId) const
            {
            return (iconId.has_value() && iconId.value() < GetImageList().size() &&
                    GetImageList()[iconId.value()].IsOk());
            }

        // events
        void OnPaint([[maybe_unused]] wxPaintEvent& event);
        void OnMouseChange(const wxMouseEvent& event);
        void OnMouseClick(const wxMouseEvent& event);
        void OnMouseLeave([[maybe_unused]] wxMouseEvent& event);
        void OnChar(wxKeyEvent& event);
        void OnResize(wxSizeEvent& event);
        void OnDraw(wxDC& dc) final;

        /** @brief Renders a glassy surface across a box.
            @param dc The device context to render on.
            @param rect The box to draw the effect on.
            @param color The base color to fill the box with.*/
        static void DrawGlassEffect(wxDC& dc, const wxRect rect, const wxColour& color);

        std::vector<wxBitmapBundle> m_imageList;

        [[nodiscard]]
        wxCoord GetItemHeight() const noexcept
            {
            return m_itemHeight;
            }

        [[nodiscard]]
        wxSize GetIconSize() const
            {
            return FromDIP(m_iconSizeDIPs);
            }

        [[nodiscard]]
        wxSize ScaleToContentSize(const wxSize sz) const
            {
            auto scaledSize{ sz };
            // for Retina display
            const double scaling = GetContentScaleFactor();

            scaledSize = wxSize{ static_cast<int>(std::lround(scaledSize.GetWidth() * scaling)),
                                 static_cast<int>(std::lround(scaledSize.GetHeight() * scaling)) };
            return scaledSize;
            }

        [[nodiscard]]
        static wxCoord GetPaddingWidth()
            {
            return wxSizerFlags::GetDefaultBorder() * 2;
            }

        [[nodiscard]]
        static wxCoord GetPaddingHeight()
            {
            return wxSizerFlags::GetDefaultBorder() * 2;
            }

        [[nodiscard]]
        static wxCoord GetSubitemIndentation()
            {
            return GetPaddingWidth() * 2;
            }

        wxCoord m_itemHeight{ 0 };
        std::optional<size_t> m_highlightedFolder{ std::nullopt };
        std::optional<size_t> m_lastSelectedFolder{ std::nullopt };
        // the folder index, and the index of its subitem
        std::pair<std::optional<size_t>, std::optional<size_t>> m_folderWithHighlightedSubitem{
            std::make_pair(std::nullopt, std::nullopt)
        };
        std::optional<wxRect> m_highlightedRect{ std::nullopt };
        std::optional<size_t> m_selectedFolder{ std::nullopt };
        std::optional<size_t> m_savedSelectedItem{ std::nullopt };
        SidebarStyle m_style{ SidebarStyle::Flat };

        struct SideBarStateInfo
            {
            explicit SideBarStateInfo(const SideBarItem& that) noexcept
                : m_id(that.m_id), m_selectedItem(that.m_selectedItem),
                  m_isExpanded(that.m_isExpanded)
                {
                }

            [[nodiscard]]
            bool operator<(const SideBarStateInfo& that) const noexcept
                {
                return m_id < that.m_id;
                }

            wxWindowID m_id{ wxID_ANY };
            std::optional<size_t> m_selectedItem{ std::nullopt };
            bool m_isExpanded{ true };
            };

        std::set<SideBarStateInfo> m_stateInfo;

        std::vector<SideBarItem> m_folders;

        wxSize m_iconSizeDIPs{ 16, 16 };

        wxColour m_selectedColor{ wxColour(L"#FDB759") };
        wxColour m_selectedFontColor{ wxColour{ 0, 0, 0 } };
        wxColour m_highlightColor{ wxColour(253, 211, 155) };
        wxColour m_highlightFontColor{ wxColour{ 0, 0, 0 } };
        wxColour m_parentColor{ wxColour(180, 189, 207) };

        bool m_highlightedIsSelected{ false };
        bool m_previouslyHighlightedItemsIsSelected{ false };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_SIDEBAR_H
