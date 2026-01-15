/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_CANVAS_H
#define WISTERIA_CANVAS_H

#include "../ui/dialogs/imageexportdlg.h"
#include "../ui/dialogs/radioboxdlg.h"
#include "graphitems.h"
#include "image.h"
#include "label.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <wx/bitmap.h>
#include <wx/clipbrd.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/dcps.h>
#include <wx/dcsvg.h>
#include <wx/dragimag.h>
#include <wx/event.h>
#include <wx/filename.h>
#include <wx/html/helpctrl.h>
#include <wx/image.h>
#include <wx/printdlg.h>
#include <wx/quantize.h>
#include <wx/timer.h>
#include <wx/wupdlock.h>
#include <wx/wx.h>

/// @cond DOXYGEN_IGNORE
wxDECLARE_EVENT(wxEVT_WISTERIA_CANVAS_DCLICK, wxCommandEvent);

#define EVT_WISTERIA_CANVAS_DCLICK(winId, fn)                                                      \
    wx__DECLARE_EVT1(wxEVT_WISTERIA_CANVAS_DCLICK, winId, wxCommandEventHandler(fn))

/// @endcond

namespace Wisteria
    {
    /// @private
    /// @brief This is only used by canvases to properly measure how much space
    ///     an embedded object needs.
    /// @sa Canvas::CalcMinWidthProportion() and Canvas::CalcMinHeightProportion().
    class CanvasItemScalingChanger
        {
      public:
        /// @brief Constructor; switches an object to use its initial scaling
        ///     when it was added to the canvas.
        /// @param object The object to change.
        /// @sa Canvas::SetFixedObject() for when an object's initial scaling is cached.
        explicit CanvasItemScalingChanger(GraphItems::GraphItemBase& object)
            : m_obj(object), m_originalScaling(object.GetScaling())
            {
            m_obj.SetScaling(m_obj.GetOriginalCanvasScaling());
            }

        /// @brief Destructor; resets the object back to its original scaling.
        ~CanvasItemScalingChanger() { m_obj.SetScaling(m_originalScaling); }

      private:
        GraphItems::GraphItemBase& m_obj;
        double m_originalScaling{ 1.0 };
        };

    /// @brief %Canvas for drawing fixed and movable objects.
    class Canvas final : public wxScrolledWindow
        {
        wxDECLARE_DYNAMIC_CLASS(Canvas);

      public:
        friend class ReportPrintout;
        friend class FitToSaveOptionsChanger;

        /// @brief Class describing a row of items on the canvas.
        class CanvasRowInfo
            {
          public:
            /// @brief Constructor.
            /// @param prop The proportion of the canvas's height that this row consumes.
            explicit CanvasRowInfo(const double prop) : m_heightProportion(prop) {}

            /// @returns The height proportion of the canvas that this row consumes.
            [[nodiscard]]
            double GetHeightProportion() const noexcept
                {
                return m_heightProportion;
                }

            /// @brief The proportion of the canvas's height that this row consumes.
            /// @param prop The height proportion (0.0 - 1.0).
            /// @returns A self reference.
            CanvasRowInfo& HeightProportion(const double prop) noexcept
                {
                m_heightProportion = prop;
                return *this;
                }

            /// @returns @c true if row's height proportion is relative to the entire
            ///     canvas, regardless of canvas padding and titles.
            [[nodiscard]]
            bool IsProportionLocked() const noexcept
                {
                return m_lockProportion;
                }

            /// @brief If @c true, the row's height proportion is relative to the entire
            ///     canvas, regardless of canvas padding and titles.
            /// @details This is useful for horizontally placed legends, so that
            ///     their scaling doesn't grow too small when canvas titles are added.
            /// @param locked Whether row's proportion is relative to the entire canvas
            ///     and remains locked, regardless of titles.
            /// @warning This should only be used for the first or last row on a page,
            ///     as it requires to adjust the layout of previous items on the page.
            ///     If multiple items are locked on the same page or appear in the middle
            ///     of the page, this will result in unexpected page layout.
            /// @returns A self reference.
            CanvasRowInfo& LockProportion(const bool locked) noexcept
                {
                m_lockProportion = locked;
                return *this;
                }

            /// @returns The number of rows that this row should consume.
            [[nodiscard]]
            size_t GetRowCount() const noexcept
                {
                return m_rowCount;
                }

            /// @brief Sets the number of rows that this row should consume.
            /// @param rCount The number of rows.
            /// @returns A self reference.
            CanvasRowInfo& RowCount(const size_t rCount) noexcept
                {
                m_rowCount = rCount;
                return *this;
                }

          private:
            double m_heightProportion{ 1.0 };
            bool m_lockProportion{ false };
            size_t m_rowCount{ 1 };
            };

        /** @brief Constructor.
            @param parent The parent window that will manage the canvas.
            @param itemId The ID of this canvas.
            @param pos The position.
            @param size The initial size of the canvas; will change size relative to @c parent.
            @param flags Window flags passed to wxScrolledWindow.*/
        explicit Canvas(wxWindow* parent, int itemId = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                        long flags = 0);
        /// @private
        Canvas() = default;
        /// @private
        Canvas(const Canvas&) = delete;
        /// @private
        Canvas& operator=(const Canvas&) = delete;

        /// @private
        void OnDraw(wxDC& dc) final;

        // standard events
        /// @private
        void OnSave([[maybe_unused]] wxCommandEvent& event);
        /// @private
        /// @warning Only available on Windows.
        void OnPreview([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnPrint([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnCopy([[maybe_unused]] wxCommandEvent& event);

        /** @name Background Functions
            @brief Functions related to customizing the canvas's background.*/
        /// @{

        /// @returns The background color of the canvas.
        [[nodiscard]]
        const wxColour& GetBackgroundColor() const noexcept
            {
            return m_bgColor;
            }

        /** @brief Sets the background color of the canvas.
            @param color The color to apply to the background.
            @param includeLinearGradient Whether to apply this color as a linear gradient
                (going towards white) across the background.*/
        void SetBackgroundColor(const wxColor& color, const bool includeLinearGradient = false)
            {
            if (color.IsOk())
                {
                m_bgColor = color;
                m_bgColorUseLinearGradient = includeLinearGradient;
                }
            }

        /** @brief Sets the background image being drawn on the canvas.
            @param backgroundImage The image to draw on the background.
            @param opacity The opacity to render the image with.
            @code
             // load an SVG file and scale it up to the default size of a canvas
             wxGCDC gdc(this);
             const auto sz = Image::ToBestSize(
                                Image::GetSVGSize(L"logo.svg"),
                                gdc.FromDIP(wxSize(Canvas::GetDefaultCanvasWidthDIPs(),
                                                   Canvas::GetDefaultCanvasHeightDIPs())));
             const auto bb = wxBitmapBundle::FromSVGFile(L"logo.svg", sz);
             // now, set it as the canvas's background
             canvas->SetBackgroundImage(bb);
            @endcode*/
        void SetBackgroundImage(const wxBitmapBundle& backgroundImage,
                                uint8_t opacity = wxALPHA_OPAQUE) noexcept;

        /// @}

        /** @name Watermark Functions
            @brief Functions related to displaying a watermarked stamp or text on the canvas.*/
        /// @{

        /// @brief Overlays translucent text diagonally across the canvas.
        /// @param watermark The text to draw as the watermark (e.g., a copyright notice).
        /// @sa SetWatermarkColor().
        void SetWatermark(const wxString& watermark) { m_watermark = watermark; }

        /// @returns The watermark label shown across the canvas.
        /// @note The tags `@DATETIME@`, `@DATE@`, and `@TIME@` are expanded to their literal
        ///     values at time of rendering.
        [[nodiscard]]
        wxString GetWatermark() const;

        /// @brief Sets the color of the watermark.
        /// @param color The color of the watermark.
        /// @note Transparency of this color will be used. If opaque, then the default
        ///     transparency will be applied at runtime.
        /// @sa SetWatermark().
        void SetWatermarkColor(const wxColour& color) { m_watermarkColor = color; }

        /// @returns The color of the watermark.
        [[nodiscard]]
        wxColour GetWatermarkColor() const
            {
            return m_watermarkColor;
            }

        /// @brief Overlays a translucent image on bottom corner of the canvas.
        /// @param watermark The image to draw as a watermark (e.g., a company logo).
        /// @param sz The suggested size of the watermark (in DIPs).\n
        ///     The image's aspect ratio will be maintained, one of the dimensions from @c sz
        ///     may be adjusted to be smaller.
        void SetWatermarkLogo(const wxBitmapBundle& watermark, const wxSize sz) noexcept
            {
            m_watermarkImg = watermark;
            m_watermarkImgSizeDIPs = sz;
            }

        /// @}

        /** @name Size Functions
            @brief Functions related to width and height measurements of the canvas.*/
        /// @{

        /// @brief Sets the aspect ratio of the canvas to its printer settings.
        /// @details Call this after setting the paper size and orientation via
        ///     SetPrinterSettings().
        /// @sa SetPrinterSettings(), MaintainAspectRatio(), FitToPageWhenPrinting().
        void SetSizeFromPaperSize();

        /// @returns @c true if the aspect ratio of the drawing area is maintained when the window
        ///     is resized.
        [[nodiscard]]
        bool IsMaintainingAspectRatio() const noexcept
            {
            return m_maintainAspectRatio;
            }

        /// @brief Set to @c true for the drawing area to maintain its aspect ratio when the
        ///     window is resized.
        /// @details The aspect ratio is controlled by
        ///     SetCanvasMinWidthDIPs()/SetCanvasMinHeightDIPs()
        ///     or SetSizeFromPaperSize().\n
        ///     This is useful for reports (i.e., canvases with multiple items on it).
        /// @param maintain @c true to maintain the aspect ratio when resizing.
        void MaintainAspectRatio(const bool maintain) noexcept { m_maintainAspectRatio = maintain; }

        /// @returns The minimum width that the canvas can be. It will be forced to be this
        ///     wide even as its parent is resized.
        [[nodiscard]]
        int GetCanvasMinWidthDIPs() const noexcept
            {
            return m_canvasMinSizeDIPs.GetWidth();
            }

        /** @brief Sets the minimum height that the canvas can be. It will be forced to be
                this tall even as its parent is resized.
            @param minWidth The minimum width to use.*/
        void SetCanvasMinWidthDIPs(const int minWidth) noexcept
            {
            m_canvasMinSizeDIPs.SetWidth(minWidth);
            }

        /// @returns The minimum height that the canvas can be. It will be forced to be this
        ///     tall even as its parent is resized.
        [[nodiscard]]
        int GetCanvasMinHeightDIPs() const noexcept
            {
            return m_canvasMinSizeDIPs.GetHeight();
            }

        /** @brief Sets the minimum height that the canvas can be. It will be forced to be this
                tall even as its parent is resized.
            @param minHeight The minimum height to use.*/
        void SetCanvasMinHeightDIPs(int minHeight);

        /// @returns The default minimum width used for canvas.
        ///     Can be overridden by SetCanvasMinWidthDIPs().
        [[nodiscard]]
        static int GetDefaultCanvasWidthDIPs() noexcept
            {
            return m_defaultWidthDIPs;
            }

        /// @returns The default minimum height used for canvas.
        ///     Can be overridden by SetCanvasMinHeightDIPs().
        [[nodiscard]]
        static int GetDefaultCanvasHeightDIPs() noexcept
            {
            return m_defaultHeightDIPs;
            }

        /** @brief Calculates the minimum percent of the canvas an item should consume
                when at 1.0 scaling.
            @param item The item to measure.
            @returns The percent of the canvas the item may need.
            @details This should be passed to the item's @c SetCanvasWidthProportion()
                method prior to adding to the canvas.\n
                Also, if this object needs canvas margins around it, set those to the
                object prior to calling this because those are factored into this calculation.
            @note This is a low-level function. Prefer using CalcRowDimensions() instead.*/
        [[nodiscard]]
        double CalcMinWidthProportion(Wisteria::GraphItems::GraphItemBase& item)
            {
            wxGCDC gdc(this);
            // switch the object to (temporarily) use its original scaling from the client
            // (not the current window's scaling) so that we measure it correctly against
            // a 1.0 scaled canvas
            const CanvasItemScalingChanger sc(item);
            // also, reset any previous min size information for a call to the object's
            // SetBoundingBox(), as we will be resizing this item from scratch
            item.SetMinimumUserSizeDIPs(std::nullopt, std::nullopt);
            item.RecalcSizes(gdc);
            return std::min(
                1.0, safe_divide<double>(item.GetBoundingBox(gdc).GetWidth() +
                                             // canvas margins are not part of the bounding box
                                             // calculation, so those need to be factored in here
                                             gdc.FromDIP(item.GetLeftCanvasMargin()) +
                                             gdc.FromDIP(item.GetRightCanvasMargin()),
                                         gdc.FromDIP(GetCanvasMinWidthDIPs())));
            }

        /** @brief Calculates the minimum percent of the canvas an item should consume
                when at @c 1.0 scaling.
            @param item The item to measure.
            @returns The percent of the canvas the item may need.
            @details This can be passed to the canvas's `GetRowInfo().HeightProportion()` method.
                Also, if this object needs canvas margins around it, set those to the
                object prior to calling this because those are factored into this calculation.
            @note This is a low-level function. Prefer using CalcRowDimensions() instead.*/
        [[nodiscard]]
        double CalcMinHeightProportion(Wisteria::GraphItems::GraphItemBase& item);

        /// @brief Calculates the proportions of the canvas that each row and column
        ///     should consume.
        /// @details This takes into account items (and rows) whose content size will
        ///     determine the height of the rows and the widths of their columns.\n
        ///     This will also lock rows' proportions if their heights are controlled
        ///     by their content, which is useful for when the aspect ratio of the
        ///     canvas is changed.
        /// @note This should be called while the canvas is being built and
        ///     after all items have been added to it (via SetFixedObject()).
        ///     Likewise, this should be called after changing the canvas's aspect
        ///     ratio (via SetCanvasMinWidthDIPs()/SetCanvasMinHeightDIPs()).
        ///     It will have no effect if called after the windowing framework has taken
        ///     control of it (i.e., being resized).\n
        void CalcRowDimensions();
        /// @}

        /** @name Layout Functions
            @brief Functions related to the fixed objects (e.g., plots) being embedded
                onto the canvas. These items are placed within a grid and accessed and
                managed through that.*/
        /// @{

        /** @brief Sets the number of rows and columns in the grid of fixed objects on the canvas.
            @param rows The number of rows to use.
            @param columns The number of columns to use.
            @note Contents currently in the grid will be preserved,
                unless @c rows or @c columns is smaller than the current size.\n
                Call ClearFixedObjects() if you need to clear the current contents.*/
        void SetFixedObjectsGridSize(size_t rows, size_t columns);
        /// @returns The size of the fixed object grid (number of rows x columns).
        [[nodiscard]]
        std::pair<size_t, size_t> GetFixedObjectsGridSize() const;

        /// @brief Removes all fixed objects and sets the grid back to (0, 0).
        void ClearFixedObjects() { m_fixedObjects.clear(); }

        /** @brief Sets the fixed object at the given row and column.
            @param row The row location of the item being set.
            @param column The column location of the item being set.
            @param object The object being added to the location.*/
        void SetFixedObject(size_t row, size_t column,
                            const std::shared_ptr<GraphItems::GraphItemBase>& object);
        /// @returns The fixed object at @c row and @c column.
        /// @param row The row of the object.
        /// @param column The column of the object.
        [[nodiscard]]
        std::shared_ptr<GraphItems::GraphItemBase> GetFixedObject(size_t row, size_t column);
        /// @returns The fixed object with the given ID, or null if not found.
        /// @param itemId The ID of the object to look for.
        [[nodiscard]]
        std::shared_ptr<GraphItems::GraphItemBase> FindFixedObject(long itemId) noexcept;

        /** @brief Whether to align the content of items across each row.
            @details For example, this will set the Y axes of the plots
                 (as well as stand-alone axes) across a row to have the same height and positions.
            @param align @c true to align row contents.*/
        void AlignRowContent(const bool align) noexcept { m_alignRowContent = align; }

        /// @returns @c true if items (e.g., plots, common axes) are having their content
        ///     aligned with each other across each row.
        [[nodiscard]]
        bool IsRowContentAligned() const noexcept
            {
            return m_alignRowContent;
            }

        /** @brief Whether to align the content of items down each column.
            @details Items (e.g., graphs) will have their content adjusted
                to the width and positions of the smallest item (that supports `GetContentRect()`)
                from the column. (Items not overriding `GetContentRect()` will be ignored.)\n
                As an example use case, this will set the X axes of the plots
                (as well as stand-alone axes) down a column to have the same width and positions.
            @param align @c true to align column contents.
            @note If the grid is jagged, then the content alignment will stop on the first
                row that has fewer columns than the top row. For example, if a canvas has three
                rows, where the first and last rows have two graphs and the second only has
                one graph, then the X axes of the first column of graphs will be aligned,
                but not the second column.
                This is because the second row does not have a second column, so alignment adjusting
                stops at that point.*/
        void AlignColumnContent(const bool align) noexcept { m_alignColumnContent = align; }

        /// @returns @c true if items (e.g., plots, common axes) are having their content
        ///     aligned with each other down each column.
        [[nodiscard]]
        bool IsColumnContentAligned() const noexcept
            {
            return m_alignColumnContent;
            }

        /** @brief Accesses attributes for a row.
            @details This can be used for adjusting the row's canvas height proportion.
            @param row The row index to access.
            @returns The row information at the specified index.
            @warning When setting a row's proportion, be sure to reset all other row proportions
                as well; otherwise, they will not all add up to 100%.
                This is because when a grid size is specified, the rows are given
                uniform proportions. If one is changes, then the sum of all row proportions
                will be more or less than 100%. You must adjust all other rows
                accordingly.*/
        [[nodiscard]]
        CanvasRowInfo& GetRowInfo(const size_t row)
            {
            assert(row < m_rowsInfo.size() && L"Invalid row in call to GetRowInfo()!");
            return m_rowsInfo.at(row);
            }

        /// @}

        /** @name Free-floating objects Functions
            @brief Functions related to the floating objects
                (e.g., a label used as a "sticky note") being shown on the canvas.\n
                These items are not part of the fixed object grid, but instead placed
                anywhere on the canvas, sitting on top of the grid.*/
        /// @{

        /** @brief Gets/sets the free floating (i.e., movable) objects on the canvas.
            @note These items are never cleared by the canvas itself and are not connected
                to the anything. When the canvas is resized, the size and position of these
                items do <b>not</b> change. Also, the canvas takes ownership of any objects
                added to this collection.
            @returns The free-floating objects.*/
        [[nodiscard]]
        std::vector<std::shared_ptr<GraphItems::GraphItemBase>>& GetFreeFloatingObjects() noexcept
            {
            return m_freeFloatingObjects;
            }

        /// @}

        /** @name Title Functions
            @brief Functions related to titles around the canvas.*/
        /// @{

        /// @returns The top titles. This can be used to add or edit a top title.
        /// @note Call @c Label::SetRelativeAlignment() to adjust the alignment of the
        ///     title on the canvas.
        [[nodiscard]]
        std::vector<GraphItems::Label>& GetTopTitles() noexcept
            {
            return m_topTitles;
            }

        /// @returns The bottom titles. This can be used to add or edit a bottom title.
        [[nodiscard]]
        std::vector<GraphItems::Label>& GetBottomTitles() noexcept
            {
            return m_bottomTitles;
            }

        /// @returns The left titles. This can be used to add or edit a left title.
        [[nodiscard]]
        std::vector<GraphItems::Label>& GetLeftTitles() noexcept
            {
            return m_leftTitles;
            }

        /// @returns The right titles. This can be used to add or edit a right title.
        [[nodiscard]]
        std::vector<GraphItems::Label>& GetRightTitles() noexcept
            {
            return m_rightTitles;
            }

        /// @}

        /** @name Zoom Functions
            @brief Functions related to zooming in and out of the canvas.*/
        /// @{

        /// @brief Zooms in (i.e., increases the scaling) of the canvas.
        void ZoomIn();
        /// @brief Zooms out (i.e., decreases the scaling) of the canvas.
        void ZoomOut();
        /// @brief Resets the scaling of the canvas to the default.
        void ZoomReset();

        /// @}

        /** @name Print Functions
            @brief Functions related to printing and printer settings.
            @sa The [printing](../../Printing.md) overview for more information.*/
        /// @{

        /** @brief Sets the printer data, which includes paper size, orientation, etc.
            @param printData The printer data (i.e., system print settings).*/
        void SetPrinterSettings(const wxPrintData& printData) noexcept { m_printData = printData; }

        /** @brief Access the printer data. This is useful for changing the print
                settings for the canvas (e.g., changing the paper orientation).
            @returns The printer data.*/
        [[nodiscard]]
        wxPrintData& GetPrinterSettings() noexcept
            {
            return m_printData;
            }

        /// @returns @c true If fitting the canvas's content to the full page when printing.
        [[nodiscard]]
        bool IsFittingToPageWhenPrinting() const noexcept
            {
            return m_fitToPageWhenPrinting;
            }

        /** @brief Adjusts the canvas's content to fit the page when printing.
            @details The default is to draw the content as-is onto the paper,
                maintaining its aspect ratio. (This aspect ratio is controlled
                by calling SetCanvasMinWidthDIPs() and SetCanvasMinHeightDIPs().)\n
                Setting this to @c true will adjust the canvas's aspect ratio
                to fit the paper's size, resulting in filling the entire page.
            @param fit @c true to fit the canvas to the entire printed page.*/
        void FitToPageWhenPrinting(const bool fit) noexcept { m_fitToPageWhenPrinting = fit; }

        /// @brief Sets the left printer header string.
        /// @param header The string to set as the left header (when printed).
        void SetLeftPrinterHeader(const wxString& header) { m_leftPrinterHeader = header; }

        /// @returns The left printer header string.
        [[nodiscard]]
        const wxString& GetLeftPrinterHeader() const noexcept
            {
            return m_leftPrinterHeader;
            }

        /// @brief Sets the center printer header string.
        /// @param header The string to set as the center header (when printed).
        void SetCenterPrinterHeader(const wxString& header) { m_centerPrinterHeader = header; }

        /// @returns The center printer header string.
        [[nodiscard]]
        const wxString& GetCenterPrinterHeader() const noexcept
            {
            return m_centerPrinterHeader;
            }

        /// @brief Sets the right printer header string.
        /// @param header The string to set as the right header (when printed).
        void SetRightPrinterHeader(const wxString& header) { m_rightPrinterHeader = header; }

        /// @returns The right printer header string.
        [[nodiscard]]
        const wxString& GetRightPrinterHeader() const noexcept
            {
            return m_rightPrinterHeader;
            }

        /// @brief Sets the left printer footer string.
        /// @param footer The string to set as the left footer (when printed).
        void SetLeftPrinterFooter(const wxString& footer) { m_leftPrinterFooter = footer; }

        /// @returns The left printer footer string.
        [[nodiscard]]
        const wxString& GetLeftPrinterFooter() const noexcept
            {
            return m_leftPrinterFooter;
            }

        /// @brief Sets the center printer footer string.
        /// @param footer The string to set as the center footer (when printed).
        void SetCenterPrinterFooter(const wxString& footer) { m_centerPrinterFooter = footer; }

        /// @returns The center printer footer string.
        [[nodiscard]]
        const wxString& GetCenterPrinterFooter() const noexcept
            {
            return m_centerPrinterFooter;
            }

        /// @brief Sets the right printer footer string.
        /// @param footer The string to set as the right footer (when printed).
        void SetRightPrinterFooter(const wxString& footer) { m_rightPrinterFooter = footer; }

        /// @returns The right printer footer string.
        [[nodiscard]]
        const wxString& GetRightPrinterFooter() const noexcept
            {
            return m_rightPrinterFooter;
            }

        /// @}

        /// @brief Calculates the sizes of all objects on the canvas.
        /// @param dc The DC to measure content with.
        /// @details Call this if changes have been made to a subobject
        ///     (e.g., a plot) and you wish to refresh the content.
        void CalcAllSizes(wxDC& dc);

        /** @brief The scaling of the canvas's size compared to the minimum size.
            @details This is used to see how much fonts and lines need to be increased to match the
                current zoom level (i.e., the window size compared to the minimum dimensions).
            @returns The scaling.*/
        [[nodiscard]]
        double GetScaling() const
            {
            // aspect ratio is the same when resizing or zooming the window,
            // so using width or height is interchangeable here
            return std::max<double>(
                safe_divide<double>(GetCanvasRectDIPs().GetWidth(), GetCanvasMinWidthDIPs()), 1.0);
            }

        /// @brief Saves the canvas as an image.
        /// @param filePath The file path of the image to save to.
        /// @param options The export options for the image.
        /// @returns @c true upon successful saving.
        bool Save(const wxFileName& filePath, const UI::ImageExportOptions& options);

        /** @brief Sets the resources to use for the export dialog.
            @details These are set by the parent application to connect icons and
                help topics to the export dialog.
            @param helpProjectPath The path to the help file.
            @param topicPath The path to the topic in the help file.*/
        void SetExportResources(const wxString& helpProjectPath, const wxString& topicPath)
            {
            m_helpProjectPath = helpProjectPath;
            m_exportHelpTopic = topicPath;
            }

        /// @returns @c true if size events will wait half a second to recalculate and
        ///     render the canvas.
        [[nodiscard]]
        bool IsResizingDelayed() const noexcept
            {
            return m_delayResize;
            }

        /** @brief Delays resize operations by half a second to ensure that numerous,
                successive size events aren't being called constantly.
            @details This is useful for when a client is resizing a canvas with their mouse;
                This will ensure that the resize event won't resize until they start dragging
                the window for at least half a second.\n
                The default is for this to be enabled.
            @param delay @c true to delay resize events. @c false will cause every size event to
                recalculate and render the canvas.*/
        void DelayResizing(const bool delay) { m_delayResize = delay; }

        /// @private
        /// @brief If delaying resizing, this turns that off for the next (and only the next)
        ///     size event.
        /// @details This is useful for preventing a delayed resizing when the canvas is being
        ///     inserted into a parent (e.g., splitter window).
        void ResetResizeDelay() { m_blockResize = false; }

        /// @brief Direction of label drawn across a canvas.
        enum class WatermarkDirection
            {
            Horizontal, /*!< Draw text horizontally.*/
            Diagonal    /*!< Draw text diagonally.*/
            };

        /// @brief Information for drawing a watermark across a canvas.
        struct Watermark
            {
            /** @brief The text.*/
            wxString m_label;
            /** @brief The text color.*/
            wxColour m_color{ wxColour(255, 0, 0, 125) };
            /// @brief The direction that the text is drawn.
            WatermarkDirection m_direction{ WatermarkDirection::Diagonal };
            };

        /** @brief Draws a watermark label across a canvas.
            @param dc The device context to draw on.
            @param drawingRect The rect within the DC to draw within.
            @param watermark The label to draw across the canvas.
            @param scaling The scaling (e.g., zoom level) of what is being drawn on.*/
        static void DrawWatermarkLabel(wxDC& dc, wxRect drawingRect, const Watermark& watermark,
                                       double scaling);
        /** @brief Draws a watermark logo on the corner of a canvas.
            @param dc The device context to draw on.*/
        void DrawWatermarkLogo(wxDC& dc) const;

        /// @returns The (scaled) rectangle area of the canvas.
        /// @param dc The measuring DC.
        [[nodiscard]]
        wxRect GetCanvasRect(const wxDC& dc) const noexcept
            {
            wxRect scaledRect(m_rectDIPs);
            scaledRect.SetSize(dc.FromDIP(m_rectDIPs.GetSize()));
            return scaledRect;
            }

        /// @private
        [[nodiscard]]
        const wxPrintData& GetPrinterSettings() const noexcept
            {
            return m_printData;
            }

        /// @private
        [[nodiscard]]
        const std::vector<GraphItems::Label>& GetTopTitles() const noexcept
            {
            return m_topTitles;
            }

        /// @private
        [[nodiscard]]
        const std::vector<GraphItems::Label>& GetBottomTitles() const noexcept
            {
            return m_bottomTitles;
            }

        /// @private
        [[nodiscard]]
        const std::vector<GraphItems::Label>& GetLeftTitles() const noexcept
            {
            return m_leftTitles;
            }

        /// @private
        [[nodiscard]]
        const std::vector<GraphItems::Label>& GetRightTitles() const noexcept
            {
            return m_rightTitles;
            }

        /// @private
        [[nodiscard]]
        std::shared_ptr<GraphItems::GraphItemBase> GetFixedObject(size_t row, size_t column) const;

      private:
        /// @brief Divides the width of a row into columns, taking into account items
        ///     whose width should not be more than its content (at default scaling).
        /// @param row The row to calculate.
        void CalcColumnWidths(size_t row);

        /// @returns The background image being drawn on the canvas.
        [[nodiscard]]
        wxBitmapBundle& GetBackgroundImage() noexcept
            {
            return m_bgImage;
            }

        /** @brief Draws the left titles.
            @returns How much of the left margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the left margin.*/
        [[nodiscard]]
        long CalcLeftTitles(wxDC& dc, long spacingWidth);
        /** @brief Draws the right titles.
            @returns How much of the right margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the right margin.*/
        [[nodiscard]]
        long CalcRightTitles(wxDC& dc, long spacingWidth);
        /** @brief Draws the top titles.
            @returns How much of the top margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the top margin.*/
        [[nodiscard]]
        long CalcTopTitles(wxDC& dc, long spacingWidth);
        /** @brief Draws the bottom titles.
            @returns How much of the bottom margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the bottom margin.*/
        [[nodiscard]]
        long CalcBottomTitles(wxDC& dc, long spacingWidth);

        /** @brief Contrasts a title label against the canvas.
            @param[in,out] title The title label to contrast.*/
        void ContrastTitleLabel(GraphItems::Label& title) const;

        /// @returns The top-level floating (i.e., not anchored) object on the
        ///     canvas located at @c pt.
        /// @param pt The point to look at.
        /// @param dc The rendering DC.
        [[nodiscard]]
        std::vector<std::shared_ptr<GraphItems::GraphItemBase>>::reverse_iterator
        FindFreeFloatingObject(const wxPoint& pt, wxDC& dc);

        enum class DragMode
            {
            DraggingNone,
            DragStart,
            Dragging
            };

        /// @brief Apply a DC's DPI and parent canvas scaling to a value.
        /// @param value The value (e.g., pen width) to scale.
        /// @param dc The DC to measure with. May be a screen or export DC.
        /// @returns The scaled value.
        /// @note This should be used to rescale pixel values used for line widths and point sizes.
        ///     It should <b>not</b> be used with font point sizes because DPI scaling is handled by
        ///     the OS for those. Instead, font sizes should only be scaled to the canvas's scaling.
        [[nodiscard]]
        double ScaleToScreenAndCanvas(const double value, const wxDC& dc) const noexcept
            {
            return value * GetScaling() * dc.FromDIP(1);
            }

        /// @returns The rectangle area of the canvas.
        [[nodiscard]]
        const wxRect& GetCanvasRectDIPs() const noexcept
            {
            return m_rectDIPs;
            }

        // Events
        void OnResize(wxSizeEvent& event);
        void OnPaint([[maybe_unused]] wxPaintEvent& event);
        void OnContextMenu([[maybe_unused]] wxContextMenuEvent& event);
        void OnMouseEvents(wxMouseEvent& event);
        void OnKeyDown(wxKeyEvent& event);

        [[nodiscard]]
        auto& GetFixedObjects() noexcept
            {
            return m_fixedObjects;
            }

        [[nodiscard]]
        const auto& GetFixedObjects() const noexcept
            {
            return m_fixedObjects;
            }

        [[nodiscard]]
        std::vector<std::unique_ptr<GraphItems::Label>>& GetTitles() noexcept
            {
            return m_titles;
            }

        [[nodiscard]]
        const std::vector<std::unique_ptr<GraphItems::Label>>& GetTitles() const noexcept
            {
            return m_titles;
            }

        constexpr static double ZOOM_FACTOR{ 1.5 };
        int m_zoomLevel{ 0 };

        // the current drawing rect
        wxRect m_rectDIPs;
        // the minimum size of the canvas
        wxSize m_canvasMinSizeDIPs{ 0, 0 };
        constexpr static int m_defaultWidthDIPs{ 700 };
        constexpr static int m_defaultHeightDIPs{ 500 };

        bool m_maintainAspectRatio{ false };

        bool m_alignRowContent{ false };
        bool m_alignColumnContent{ false };

        wxMenu m_menu;
        wxPrintData m_printData;
        bool m_fitToPageWhenPrinting{ false };
        // headers
        wxString m_leftPrinterHeader;
        wxString m_centerPrinterHeader;
        wxString m_rightPrinterHeader;
        // footers
        wxString m_leftPrinterFooter;
        wxString m_centerPrinterFooter;
        wxString m_rightPrinterFooter;

        wxString m_helpProjectPath;
        wxString m_exportHelpTopic;

        // titles
        std::vector<GraphItems::Label> m_leftTitles;
        std::vector<GraphItems::Label> m_rightTitles;
        std::vector<GraphItems::Label> m_topTitles;
        std::vector<GraphItems::Label> m_bottomTitles;
        // internal container for the above titles
        std::vector<std::unique_ptr<GraphItems::Label>> m_titles;

        // embedded objects (e.g., graphs, legends)
        // Note that these persist until they are explicitly swapped or cleared by the client,
        // so being shared_ptrs is fine.
        std::vector<std::vector<std::shared_ptr<GraphItems::GraphItemBase>>> m_fixedObjects;
        std::vector<CanvasRowInfo> m_rowsInfo;

        // draggable items
        // (note that these objects must be share_ptrs because a state-based share_ptr must be
        //  used during drag events; this is OK because these are NOT destroyed during resizing,
        //  they persist during the lifetime of canvas [at least])
        std::unique_ptr<wxDragImage> m_dragImage;
        std::vector<std::shared_ptr<GraphItems::GraphItemBase>> m_freeFloatingObjects;

        // watermarks and logos
        wxString m_watermark;
        wxColour m_watermarkColor{ 255, 0, 0 };
        wxFont m_watermarkFont;
        wxBitmapBundle m_watermarkImg;
        wxSize m_watermarkImgSizeDIPs{ 100, 100 };

        // background values
        wxColour m_bgColor{ Colors::ColorBrewer::GetColor(Colors::Color::White) };
        uint8_t m_bgOpacity{ wxALPHA_OPAQUE };
        bool m_bgColorUseLinearGradient{ false };
        wxBitmapBundle m_bgImage;

        // resize state variables
        bool m_delayResize{ true }; // whether a timer should delay resizing during mouse dragging
        bool m_blockResize{ true }; // prevents resizing until timer allows it (this is state based)
        wxTimer m_resizeTimer;

        wxString m_debugInfo;
        };

    /// @private
    /// @brief Remembers a canvas's delay resizing option and resets it on destruction.
    class CanvasResizeDelayChanger
        {
      public:
        /// @brief Constructor
        /// @param canvas The canvas to change.
        explicit CanvasResizeDelayChanger(Canvas& canvas)
            : m_canvas(canvas), m_isDelaying(canvas.IsResizingDelayed())
            {
            }

        /// @brief Destructor; resets the canvas back to its delaying option.
        ~CanvasResizeDelayChanger() { m_canvas.DelayResizing(m_isDelaying); }

      private:
        Canvas& m_canvas;
        bool m_isDelaying{ true };
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_CANVAS_H
