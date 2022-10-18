/** @addtogroup Graphics
    @brief User interface classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __SIDEBAR_H__
#define __SIDEBAR_H__

#include <wx/wx.h>
#include <wx/window.h>
#include <wx/scrolwin.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/artprov.h>
#include <vector>
#include <set>
#include <algorithm>

DECLARE_EVENT_TYPE(wxEVT_SIDEBAR_CLICK, -1)
DECLARE_EVENT_TYPE(wxEVT_SIDEBAR_SHOWHIDE_CLICK, -1)

/** @breif A visually-enhanced tree control, similar to Outlook's sidebar.
    @details The control organizes data into categories (a top-level "folder")
        and items (sub-items under the categories).*/
class wxSideBar final : public wxScrolledCanvas
    {
    /// @brief Constructor.
    explicit wxSideBar(wxWindow* parent, wxWindowID id = wxID_ANY);
    wxSideBar(const wxSideBar&) = delete;
    wxSideBar(wxSideBar&&) = delete;
    wxSideBar& operator=(const wxSideBar&) = delete;
    wxSideBar& operator=(wxSideBar&&) = delete;

    struct SideBarColorScheme
        {
        wxColour m_backgroundColor;
        wxColour m_foregroundColor;
        wxColour m_activeColor;
        wxColour m_activeFontColor;
        wxColour m_parentColor;
        wxColour m_highlightColor;
        wxColour m_highlightFontColor;
        };
    enum class VisualEffect
        {
        Glass,
        Flat
        };
    struct SideBarSubItem
        {
        [[nodiscard]] bool operator<(const SideBarSubItem& that) const
            { return m_Label.CmpNoCase(that.m_Label) < 0; }
        wxString m_Label;
        wxWindowID m_id{ wxID_ANY };
        long m_iconIndex{ -1 };
        wxRect m_Rect;
        };
    struct SideBarItem
        {
        SideBarItem() noexcept : m_id(wxID_ANY), m_IconIndex(-1),
            m_highlightedItem(static_cast<size_t>(wxNOT_FOUND)), m_activeItem(static_cast<size_t>(wxNOT_FOUND)),
            m_isExpanded(false)
            {}
        void Expand() noexcept
            {
            if (m_subItems.size())
                { m_isExpanded = true; }
            }
        void Collapse() noexcept
            {
            if (m_subItems.size())
                { m_isExpanded = false; }
            }
        [[nodiscard]] size_t GetSubItemCount() const noexcept
            { return m_subItems.size(); }
        [[nodiscard]] bool IsSubItemSelected() const noexcept
            {
            return (m_activeItem != static_cast<size_t>(wxNOT_FOUND)) &&
                    m_activeItem < m_subItems.size();
            }
        void SortSubItems()
            { std::sort(m_subItems.begin(), m_subItems.end()); }
        wxString m_Label;
        wxWindowID m_id { wxID_ANY };
        long m_IconIndex{ -1 };
        wxRect m_Rect;
        std::vector<SideBarSubItem> m_subItems;
        size_t m_highlightedItem{ static_cast<size_t>(wxNOT_FOUND) };
        size_t m_activeItem{ static_cast<size_t>(wxNOT_FOUND) };
        bool m_isExpanded{ false };
        };

    void Maximize();
    void Minimize();

    /** @returns The total of (root level) items.
        @note Subitems are not included in this count.*/
    [[nodiscard]] size_t GetCategoryCount() const noexcept
        { return m_items.size(); }

    /** @brief Inserts a (root level) item.
        @param position The positions to insert the item.
        @param label The label to be displayed on the item.
        @param Id The numeric ID of the item.
        @param iconIndex The icon (index into the image list) to be displayed on the item. Set to -1 to not display an icon.*/
    void InsertItem(const size_t position, const wxString& label, const wxWindowID Id, const long iconIndex);
    /** @brief Insert a subitem under the specified root item.
        @param parentItemId The ID of the parent item to insert this under.
        @param label The label to display on the item.
        @param Id The numeric ID of the item.
        @param iconIndex The icon (index into the image list) to be displayed on the item. Set to -1 to not display an icon.
        @returns True if the item was inserted, false if not added (e.g., will fail if parent item is not found).*/
    bool InsertSubItemById(const wxWindowID parentItemId, const wxString& label, 
                           const wxWindowID Id, const long iconIndex);
    /** @brief Gets the width (label, icon, and padding) of a given root item.
        @details The item's subitem width are factored into this (including their margins),
            so the width of the widest subitem will be returns if wider than the root item.
        @param item The position of the root item.
        @returns The item's width.*/
    [[nodiscard]] size_t GetCategoryWidth(const size_t item);

    /** @brief Deletes all items from the sidebar.*/
    void DeleteAllCategories()
        {
        m_items.clear();
        m_highlightedItem = static_cast<size_t>(wxNOT_FOUND);
        m_activeItem = static_cast<size_t>(wxNOT_FOUND);
        CalculateSizes();
        Refresh();
        }
    /** @brief Deletes a specific (root-level) item (by index)*/
    void DeleteCategory(const size_t item)
        {
        m_items.erase(m_items.begin()+item);
        m_highlightedItem = static_cast<size_t>(wxNOT_FOUND);
        if (m_items.size() == 0)
            { m_activeItem = static_cast<size_t>(wxNOT_FOUND); }
        else if (m_activeItem >= m_items.size())
            { m_activeItem = 0; }
        CalculateSizes();
        Refresh();
        }

    /** @brief Collapses all of the items that have subitems.*/
    void CollapseAll();
    /** @brief Expands all of the items that have subitems.*/
    void ExpandAll();

    /** @brief Selects a root-level item.
        @param item The position of the item to select.
        @param setFocus Whether to set the keyboard focus to the control.
        @param sendEvent Whether to send a wxEVT_SIDEBAR_CLICK event.*/
    void SelectCategory(const size_t item, const bool setFocus = true, const bool sendEvent = true);
    /** @brief Selects a subitem.
        @param item The position of the parent item to select.
        @param subItem The position of the subitem (relative to its parent) to select.
        @param setFocus Whether to set the keyboard focus to the control.
        @param sendEvent Whether to send a wxEVT_SIDEBAR_CLICK event.*/
    void SelectSubItem(const size_t item, const size_t subItem, const bool setFocus = true, const bool sendEvent = true);
    /** @brief Selects a subitem.
        @param item The position of the parent item and subitem (relative to its parent) to select.
        @param setFocus Whether to set the keyboard focus to the control.
        @param sendEvent Whether to send a wxEVT_SIDEBAR_CLICK event.*/
    void SelectSubItem(const std::pair<size_t,size_t>& item, const bool setFocus = true, const bool sendEvent = true)
        { SelectSubItem(item.first, item.second, setFocus, sendEvent); }
    /** @brief Selects a category or subitem.
        @param item The position of the item to select.
        @param setFocus Whether to set the keyboard focus to the control.
        @param sendEvent Whether to send a wxEVT_SIDEBAR_CLICK event.*/
    void SelectAnyItem(const size_t item, const bool setFocus = true, const bool sendEvent = true);
    /** @returns The position of the selected root-level item, or wxNOT_FOUND if nothing is selected.
        @note If a subitem is selected, this will return the position of its parent item.*/
    [[nodiscard]] size_t GetSelectedCategory() const noexcept
        { return m_activeItem; }
    /** Gets the position of the selected item (or sub item).
        @return The raw position of the selected item, meaning the index of the item including subitems.*/
    [[nodiscard]] size_t GetSelectedAnyItem() const;
    /** @returns The ID of the selected root-level item, or wxNOT_FOUND if nothing is selected.
        @note If a subitem is selected, this will return the ID of its parent item.*/
    [[nodiscard]] wxWindowID GetSelectedCategoryId() const
        { return IsCategorySelected() ? m_items[GetSelectedCategory()].m_id : wxNOT_FOUND; }
    /** @brief Gets the ID of the selected subitem, or wxNOT_FOUND if no subitem is selected.
        @returns The position of the parent ID and the subitem ID.*/
    [[nodiscard]] std::pair<wxWindowID,wxWindowID> GetSelectedSubItemId() const;
    /** @returns The label of the selected category (or subitem if the category has a select subitem).*/
    [[nodiscard]] wxString GetSelectedLabel() const;
    /** @returns Whether a (root-level) item is selected in the list.*/
    [[nodiscard]] bool IsCategorySelected() const noexcept
        { return (m_activeItem != static_cast<size_t>(wxNOT_FOUND)) && m_activeItem < GetCategoryCount(); }

    /** @brief Accesses a (root-level) item (as a constant).
        @param item The item to access (by index).
        @returns The category at item index.*/
    [[nodiscard]] const SideBarItem& GetCategory(const size_t item) const
        {
        wxASSERT(item < m_items.size());
        return m_items[item];
        }
    /** @brief Accesses a (root-level) item.
        @param item The item to access (by index).
        @returns The category at item index.*/
    [[nodiscard]] SideBarItem& GetCategory(const size_t item)
        {
        wxASSERT(item < m_items.size());
        return m_items[item];
        }
    /** @returns The label of a given root item.
        @param item The index of the item in the list of root items.*/
    [[nodiscard]] wxString GetCategoryText(const size_t item) const
        {
        if (item >= GetCategoryCount())
            { return wxEmptyString; }
        return m_items[item].m_Label;
        }

    /** @brief Search for a (root-level) item by ID.
        @param Id The ID of the item to search for.
        @return@ The position of the item, or wxNOT_FOUND if not found.*/
    [[nodiscard]] long FindCategory(const wxWindowID Id) const;
    /** @brief Searches for a sub item by ID.
        @note This it will search within all of the root-level items.
        @param Id The ID of the sub item to search for.
        @returns If found, returns the parent's index and the subitem's index. Returns a
            pair of @c wxNOT_FOUND otherwise.*/
    [[nodiscard]] std::pair<long,long> FindSubItem(const wxWindowID Id) const;
    /** @brief Searches for a sub item by ID, but only within a specified parent item
        @param parentId The parent item's ID to search within.
        @param subItemId The ID of the sub item to search for.
        @returns If found, returns the parent's index and the subitem's index. Returns a
            pair of @c wxNOT_FOUND otherwise.*/
    [[nodiscard]] std::pair<long,long> FindSubItem(const wxWindowID parentId, const wxWindowID subItemId) const;
    /** Searches for a sub item by ID, but only within a specified parent item
        @param parentId The parent item's ID to search within.
        @param subItem Information about the subitem to search for. While try to find
         the best match based on @subItem's ID, label, and icon.
        @returns If found, returns the parent's index and the subitem's index. Returns a
         pair of @c wxNOT_FOUND otherwise.*/
    [[nodiscard]] std::pair<long, long> FindSubItem(const wxWindowID parentId, const SideBarSubItem& subItem) const;
        
    /** Searches for a sub item by label. Note that it will search within all of the root-level items.
        @param label The Label of the sub item to search for.
        @returns If found, returns the parent's index and the subitem's index. Returns a
         pair of wxNOT_FOUND otherwise.*/
    [[nodiscard]] std::pair<long,long> FindSubItem(const wxString& label) const;

    /** @brief Sets the control's image list.
        @param imageList A vector of bitmaps to use as the image list.*/
    void SetImageList(const std::vector<wxBitmap>& imageList)
        { m_imageList = imageList; }
    /** @returns The control's image list.*/
    [[nodiscard]] std::vector<wxBitmap>& GetImageList() noexcept
        { return m_imageList; }
    /** @returns The control's image list (constant version).*/
    [[nodiscard]] const std::vector<wxBitmap>& GetImageList() const noexcept
        { return m_imageList; }

    /** @brief Sets the color for the currently selected item.*/
    void SetActiveColour(const wxColour color) noexcept
        { m_activeColor = color; }
    /** @brief Sets the font color for the currently selected item.*/
    void SetActiveFontColour(const wxColour color) noexcept
        { m_activeFontColor = color; }
    /** @brief Sets the color for the parents.*/
    void SetParentColour(const wxColour color) noexcept
        { m_parentColor = color; }
    /** @brief Sets the color for items that are being moused over.*/
    void SetHighlightColour(const wxColour color) noexcept
        { m_highlightColor = color; }
    /** @brief Sets the font color for items that are being moused over.*/
    void SetHighlightFontColour(const wxColour color) noexcept
        { m_highlightFontColor = color; }
    /** @brief Sets the entire coloring schema of the control.
        @param colorScheme The color scheme to use.*/
    void SetColorScheme(const SideBarColorScheme& colorScheme)
        {
        SetBackgroundColour(colorScheme.m_backgroundColor);
        SetForegroundColour(colorScheme.m_foregroundColor);
        SetActiveColour(colorScheme.m_activeColor);
        SetActiveFontColour(colorScheme.m_activeFontColor);
        SetParentColour(colorScheme.m_parentColor);
        SetHighlightColour(colorScheme.m_highlightColor);
        SetHighlightFontColour(colorScheme.m_highlightFontColor);
        }

    /** @brief Sets the minimum width of the control to fit its widest item.
        @returns The new width of the control.*/
    size_t AdjustWidthToFitItems();

    /** @brief Saves information about which items are selected and expanded.*/
    void SaveState();
    /** @brief Selects and expands/collapses items previously saved from call to SaveState().*/
    void ResetState();

    /** @brief Scrolls to category (by index) if not fully visible.*/
    void EnsureCategoryVisible(const size_t category);

    /** @brief Specifies whether a small toolbar which can show/hide the control should
        be included at the top.
        @param show Set to true to include the show/hide toolbar.*/
    void IncludeShowHideToolbar(const bool show) noexcept
        { m_includeShowHideToolbar = show; }
    /// @returns Whether or not a show/hide toolbar is being displayed at the top of the control.
    [[nodiscard]] bool HasShowHideToolbar() const noexcept
        { return m_includeShowHideToolbar; }
    /// @returns True if this control is fully expanded for the user.
    /// @note This only applies if the show/hide toolbar is being displayed.
    [[nodiscard]] bool IsExpanded() const noexcept
        { return m_isExpanded; }
    /// @returns Which sort of effect is being used to render the control
    [[nodiscard]] VisualEffect GetVisualEffect() const noexcept
        { return m_effect; }
    /// @brief Sets the visual effect to render the control
    void SetVisualEffect(const VisualEffect effect) noexcept
        { m_effect = effect; }
    /// @brief Perform initial layout and size calculations.
    /// @note This should be called after filling the image list and adding all items.
    void Realize();

    /** @brief Renders a glassy surface across a box.
        @param dc The device context to render on.
        @param rect The box to draw the effect on.
        @param color The base color to fill the box with.*/
    void DrawGlassEffect(wxDC& dc, const wxRect rect, const wxColour color);
protected:
    // Updates the areas and positions for the items and returns their collective height.
    size_t CalculateItemRects();
    void CalculateSizes();
    void ClearHighlightedItems() noexcept;
    // Indicates whether an item's icon ID is valid
    bool IsValidImageId(const size_t iconId) const
        {
        return (iconId != wxNOT_FOUND &&
            iconId < GetImageList().size() &&
            GetImageList()[iconId].IsOk());
        }
    [[nodiscard]] int GetToolbarHeight() const
        {
        return HasShowHideToolbar() ?
            FromDIP(wxSize(16,16)).GetHeight() + GetPaddingHeight() : 0;
        }
private:
    // events
    void OnPaint([[maybe_unused]] wxPaintEvent& event);
    void OnMouseChange(wxMouseEvent& event);
    void OnMouseClick(wxMouseEvent& event);
    void OnMouseLeave([[maybe_unused]] wxMouseEvent& event);
    void OnDblClick(wxMouseEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnResize(wxSizeEvent& event);
    void OnDraw(wxDC& dc) final;

    std::vector<wxBitmap> m_imageList;
    [[nodiscard]] wxCoord GetItemHeight() const noexcept
        { return m_itemHeight; }
    [[nodiscard]] wxCoord GetPaddingWidth() const
        {
        return FromDIP(wxSize(wxSizerFlags::GetDefaultBorder() * 2,
                              wxSizerFlags::GetDefaultBorder() * 2)).GetWidth();
        }
    [[nodiscard]] wxCoord GetPaddingHeight() const
        {
        return FromDIP(wxSize(wxSizerFlags::GetDefaultBorder()*2,
                       wxSizerFlags::GetDefaultBorder()*2)).GetHeight();
        }
    [[nodiscard]] wxCoord GetSubitemIndentation() const
        { return GetPaddingWidth() * 2; }
    /// @returns The width of the sidebar when hidden,
    ///     which would be the width of a 16x16 icon (scaled to system's DPI)
    ///     plus the system padding.
    [[nodiscard]] wxCoord GetHideWidth() const
        { return FromDIP(wxSize(16, 16)).GetWidth() + GetPaddingWidth(); }
    wxCoord m_itemHeight{ 0 };
    size_t m_highlightedItem{ 0 };
    size_t m_activeItem{ 0 };
    size_t m_savedActiveItem{ 0 };
    VisualEffect m_effect{ VisualEffect::Flat };

    struct SideBarStateInfo
        {
        explicit SideBarStateInfo(const SideBarItem& that) noexcept : m_id(that.m_id),
            m_activeItem(that.m_activeItem),
            m_isExpanded(that.m_isExpanded)
            {}
        [[nodiscard]] bool operator<(const SideBarStateInfo& that) const noexcept
            { return m_id < that.m_id; }
        wxWindowID m_id{ wxID_ANY };
        size_t m_activeItem{ 0 };
        bool m_isExpanded{ true };
        };
    std::set<SideBarStateInfo> m_stateInfo;

    std::vector<SideBarItem> m_items;

    wxColour m_activeColor;
    wxColour m_activeFontColor;
    wxColour m_parentColor;
    wxColour m_highlightColor;
    wxColour m_highlightFontColor;

    bool m_includeShowHideToolbar{ true };
    bool m_isExpanded{ true };
    wxRect m_toolbarRect;

    wxDECLARE_EVENT_TABLE();
    };

/** @}*/

#endif //__SIDEBAR_H__
