/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_CANVAS_H__
#define __WISTERIA_CANVAS_H__

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/dcprint.h>
#include <wx/dcps.h>
#include <wx/dcgraph.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/filename.h>
#include <wx/dataobj.h>
#include <wx/clipbrd.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/file.h>
#include <wx/tokenzr.h>
#include <wx/graphics.h>
#include <wx/dragimag.h>
#include <wx/html/helpctrl.h>
#include <wx/dcsvg.h>
#include <wx/quantize.h>
#include <wx/event.h>
#include <wx/wupdlock.h>
#include <cmath>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>
#include "graphitems.h"
#include "image.h"
#include "label.h"
#include "../ui/imageexportdlg.h"
#include "../ui/radioboxdlg.h"

DECLARE_EVENT_TYPE(EVT_WISTERIA_CANVAS_DCLICK, -1)

namespace Wisteria
    {
    /// @brief %Canvas for drawing, movable objects, etc.
    class Canvas : public wxScrolledWindow
        {
    public:
        friend class CanvasPrintout;

        /// @brief Class describing a row of items on the canvas.
        class CanvasRowInfo
            {
        public:
            /// @brief Constructor.
            /// @param prop The proportion of the canvas's height that this row consumes.
            CanvasRowInfo(const double prop) : m_heightProportion(prop)
                {}
            /// @returns The height proportion of the canvas that this row consumes.
            [[nodiscard]] double GetHeightProportion() const noexcept
                { return m_heightProportion; }
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
            [[nodiscard]] bool IsProportionLocked() const noexcept
                { return m_lockProportion; }
            /// @brief If @c true, the row's height proportion is relative to the entire
            ///     canvas, regardless of canvas padding and titles.
            /// @details This is useful for horizontally placed legends, so that
            ///     their scaling doesn't grow too small when canvas titles are added.
            /// @param prop Whether row's proportion is relative to the entire canvas
            ///     and remains locked, regardless of titles.
            /// @warning This should only be used for the first or last row on a page,
            ///     as it requires to adjust the layout of previous items on the page.
            ///     If multiple items are locked on the same page or appear on the middle
            ///     of the page, this will result in unexpeced page layout.
            /// @returns A self reference.
            CanvasRowInfo& LockProportion(const bool locked) noexcept
                {
                m_lockProportion = locked;
                return *this;
                }
        private:
            double m_heightProportion{ 1.0 };
            bool m_lockProportion{ false };
            };

        /** @brief Constructor.
            @param parent The parent window that will manage the canvas.
            @param itemId The ID of this canvas.
            @param pos The position.
            @param size The initial size of the canvas; will change size relative to @c parent.
            @param flags Window flags passed to wxScrolledWindow.*/
        explicit Canvas(wxWindow* parent, int itemId = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               const long flags = 0);
        /// @private
        Canvas(const Canvas&) = delete;
        /// @private
        Canvas(Canvas&&) = delete;
        /// @private
        Canvas& operator=(const Canvas&) = delete;
        /// @private
        Canvas& operator=(Canvas&&) = delete;
        /// @private
        ~Canvas()
            {
            /* Note for developer: you can't use a shared_ptr with wxMenu, there is some odd
               copy CTOR compile error with wxMenu that prevents doing the following:

               std::make_shared<wxMenu>(new wxMenu);

               So, we need to manually manage this resource.*/
            delete m_menu;
            }

        /// @private
        void OnDraw(wxDC& dc) override;

        // standard events
        /// @private
        void OnSave([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnPreview([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnPrint([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnCopy([[maybe_unused]] wxCommandEvent& event);

        /** @name Background Functions
            @brief Functions related to customizing the canvas's background.*/
        /// @{

        /// @returns The background color of the canvas.
        [[nodiscard]] const wxColour& GetBackgroundColor() const noexcept
            { return m_bgColor; }
        /** @brief Sets the background color of the canvas.
            @param color The color to apply to the background.
            @param includeLinearGradient Whether to apply this color as a linear gradient
                (going towards white) across the background.*/
        void SetBackgroundColor(const wxColor& color,
                                const bool includeLinearGradient = false)
            {
            m_bgColor = color;
            m_bgColorUseLinearGradient = includeLinearGradient;
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
             const auto bb = wxBitmapBundle::FromSVGFile(L"logo.svg", sz));
             // now, set it as the canvas's background
             canvas->SetBackgroundImage(bb);
            @endcode*/
        void SetBackgroundImage(const wxBitmapBundle& backgroundImage,
                                const uint8_t opacity = wxALPHA_OPAQUE) noexcept;
        /// @}

        /** @name Watermark Functions
            @brief Functions related to displaying a watermarked stamp or text on the canvas.*/
        /// @{

        /// @brief Overlays translucent text diagonally across the canvas.
        /// @param watermark The text to draw as the watermark (e.g., a copyright notice).
        void SetWatermark(const wxString& watermark)
            { m_watermark = watermark; }
        /// @returns The watermark label shown across the canvas.
        /// @note The tags [DATETIME], [DATE], and [TIME] are expanded to their literal values
        ///     at time of rendering.
        [[nodiscard]] wxString GetWatermark() const;
        /// @brief Overlays a translucent image on bottom corner of the canvas.
        /// @param watermark The image to draw as a watermark (e.g., a company logo).
        /// @param sz The suggested size of the watermark (in DIPs).\n
        ///     The image's aspect ratio will be maintained, one of the dimenstions from @c sz
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

        /// @returns The minimum width that the canvas can be, it will be forced to be this
        ///     wide even as its parent is resized.
        [[nodiscard]] int GetCanvasMinWidthDIPs() const noexcept
            { return m_canvasMinSizeDIPs.GetWidth(); }
        /** @brief Sets the minimum height that the canvas can be, it will be forced to be
                this tall even as its parent is resized.
            @param minWidth The minimum width to use.*/
        void SetCanvasMinWidthDIPs(const int minWidth) noexcept
           { m_canvasMinSizeDIPs.SetWidth(minWidth); }
        /// @returns The minimum height that the canvas can be, it will be forced to be this
        ///     tall even as its parent is resized.
        [[nodiscard]] int GetCanvasMinHeightDIPs() const noexcept
           { return m_canvasMinSizeDIPs.GetHeight(); }
        /** @brief Sets the minimum height that the canvas can be, it will be forced to be this
                tall even as its parent is resized.
            @param minHeight The minimum height to use.*/
        void SetCanvasMinHeightDIPs(const int minHeight) noexcept
           { m_canvasMinSizeDIPs.SetHeight(minHeight); }
        /// @returns The default minimum width used for canvas.
        ///     Can be overridden by SetCanvasMinWidthDIPs().
        [[nodiscard]] static int GetDefaultCanvasWidthDIPs()
            { return m_defaultWidthDIPs; }
        /// @returns The default minimum height used for canvas.
        ///     Can be overridden by SetCanvasMinHeightDIPs().
        [[nodiscard]] static int GetDefaultCanvasHeightDIPs()
            { return m_defaultHeightDIPs; }
        /** @brief Calculates the minimum percent of the canvas an item should consume
                when at 1.0 scaling.
            @param item The item to measure.
            @returns The percent of the canvas the item may need.
            @note This should be passed to the items SetCanvasWidthProportion()
                method prior to adding to the canvas.*/
        [[nodiscard]] double CalcMinWidthProportion(
                                 const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& item)
            {
            wxGCDC gdc(this);
            return safe_divide<double>(
                item->GetBoundingBox(gdc).GetWidth(),
                gdc.FromDIP(GetCanvasMinWidthDIPs()));
            }
        /** @brief Calculates the minimum percent of the canvas an item should consume
                when at 1.0 scaling.
            @param item The item to measure.
            @returns The percent of the canvas the item may need.
            @note This can be passed to the canvas's `GetRowInfo().HeightProportion()` method.*/
        [[nodiscard]] double CalcMinHeightProportion(
                                 const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& item)
            {
            wxGCDC gdc(this);
            return safe_divide<double>(
                item->GetBoundingBox(gdc).GetHeight(),
                gdc.FromDIP(GetCanvasMinHeightDIPs()));
            }
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
        void SetFixedObjectsGridSize(const size_t rows, const size_t columns);
        /// @returns The size of the fixed object grid (number of rows x columns).
        [[nodiscard]] std::pair<size_t, size_t> GetFixedObjectsGridSize() const;
        /// @brief Removes all fixed objects and sets the grid back to (0, 0).
        void ClearFixedObjects()
            { m_fixedObjects.clear(); }
        /** @brief Sets the fixed object at the given row and column.
            @param row The row location of the item being set.
            @param column The column location of the item being set.
            @param object The object being add to the location.*/
        void SetFixedObject(const size_t row, const size_t column,
                            std::shared_ptr<GraphItems::GraphItemBase> object);
        /// @returns The fixed object at @c row and @c column.
        /// @param row The row of the object.
        /// @param column The column of the object.
        [[nodiscard]] std::shared_ptr<GraphItems::GraphItemBase>
                          GetFixedObject(const size_t row, const size_t column);

        /** @brief Whether to align the content of items across each row.
            @details For example, this will set the Y axes of the plots
                 as stand alone axes) across a row to have the same height and positions.
            @param align @c true to align row contents.*/
        void AlignRowContent(const bool align) noexcept
            { m_alignRowContent = align; }
        /// @returns @c true if items (e.g., plots, common axes) are having their content
        ///     aligned with each other across each row.
        [[nodiscard]] bool IsRowContentAligned() const noexcept
            { return m_alignRowContent; }

        /** @brief Whether to align the content of items across each column.
            @details For example, this will set the X axes of the plots
                (as well as stand alone axes) down a column to have the same width and positions.
            @param align @c true to align column contents.
            @note If the grid is jagged, then the content alignment will stop on the first
                row that has less columns than the top row. For example, if a canvas has three rows,
                where the first and last rows have two graphs and the second only has one graph, then
                the X axes of the first column of graphs will be aligned, but not the second column.
                This is because the second row does not have a second column, so alignment adjusting
                stops at that point.*/
        void AlignColumnContent(const bool align) noexcept
            { m_alignColumnContent = align; }
        /// @returns @c true if items (e.g., plots, common axes) are having their content
        ///     aligned with each other down each column.
        [[nodiscard]] bool IsColumnContentAligned() const noexcept
            { return m_alignColumnContent; }

        /** @brief Accesses attibutes for a row.
            @details This can be used for adjusting the row's canvas height proportion.
            @returns The row information at the specified index.
            @warning When setting a row's proportion, be sure to reset all other row proportions
                as well; otherwise, they will not all add up to 100%. This is because when a grid size
                is specified, the rows are given uniform proportions. If one is changes, then the sum
                of all row proportions will be more or less than 100%. You must adjust all other rows
                accordingly.*/
        [[nodiscard]] CanvasRowInfo& GetRowInfo(const size_t row) noexcept
            {
            wxASSERT_MSG(row < m_rowsInfo.size(),
                L"Invalid row in call to GetRowInfo()!");
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
                items does **not** change. Also, the canvas takes ownership of any objects
                added to this collection.
            @returns The free-floating objects.*/
        [[nodiscard]] std::vector<std::shared_ptr<GraphItems::GraphItemBase>>&
                          GetFreeFloatingObjects() noexcept
            { return m_freeFloatingObjects; }
        /// @}

        /** @name Title Functions
            @brief Functions related to titles around the canvas.*/
        /// @{

        /// @returns The top titles. This can be used to add or edit a top title.
        /// @note Call Label::SetRelativeAlignment() to adjust the alignment of the
        ///     title on the canvas.
        [[nodiscard]] std::vector<GraphItems::Label>& GetTopTitles() noexcept
            { return m_topTitles; }
        /// @returns The bottom titles. This can be used to add or edit a bottom title.
        [[nodiscard]] std::vector<GraphItems::Label>& GetBottomTitles() noexcept
            { return m_bottomTitles; }
        /// @returns The left titles. This can be used to add or edit a left title.
        [[nodiscard]] std::vector<GraphItems::Label>& GetLeftTitles() noexcept
            { return m_leftTitles; }
        /// @returns The right titles. This can be used to add or edit a right title.
        [[nodiscard]] std::vector<GraphItems::Label>& GetRightTitles() noexcept
            { return m_rightTitles; }
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
        void SetPrinterData(const wxPrintData& printData) noexcept
            { m_printData = printData; }
        /// @returns The printer data.
        [[nodiscard]] const wxPrintData& GetPrinterData() const noexcept
            { return m_printData; }

        /// @returns @c true If fitting the canvas's content to the full page
        ///     when printing.
        [[nodiscard]] bool IsFittingToPageWhenPrinting() const noexcept
            { return m_fitToPageWhenPrinting; }
        /** @brief Adjusts the canvas's content to fit the page when printing.
            @details The default is to draw the content as-is onto the paper,
                maintaining its aspect ratio. (This aspect ratio is controlled
                by calling SetCanvasMinWidthDIPs() and SetCanvasMinHeightDIPs().)\n
                Setting this to @c true will adjust the canvas's aspect ratio
                to fit the paper's size, resulting in filling the entire page.
            @param fit @c true to fit the canvas to the entire printed page.*/
        void FitToPageWhenPrinting(const bool fit) noexcept
            { m_fitToPageWhenPrinting = fit; }

        /// @brief Sets the left printer header string.
        /// @param header The string to set as the left header (when printed).
        void SetLeftPrinterHeader(const wxString& header)
            { m_leftPrinterHeader = header; }
        /// @returns The left printer header string.
        [[nodiscard]] const wxString& GetLeftPrinterHeader() const noexcept
            { return m_leftPrinterHeader; }

        /// @brief Sets the center printer header string.
        /// @param header The string to set as the center header (when printed).
        void SetCenterPrinterHeader(const wxString& header)
            { m_centerPrinterHeader = header; }
        /// @returns The center printer header string.
        [[nodiscard]] const wxString& GetCenterPrinterHeader() const noexcept
            { return m_centerPrinterHeader; }

        /// @brief Sets the right printer header string.
        /// @param header The string to set as the right header (when printed).
        void SetRightPrinterHeader(const wxString& header)
            { m_rightPrinterHeader = header; }
        /// @returns The right printer header string.
        [[nodiscard]] const wxString& GetRightPrinterHeader() const noexcept
            { return m_rightPrinterHeader; }

        /// @brief Sets the left printer footer string.
        /// @param footer The string to set as the left footer (when printed).
        void SetLeftPrinterFooter(const wxString& footer)
            { m_leftPrinterFooter = footer; }
        /// @returns The left printer footer string.
        [[nodiscard]] const wxString& GetLeftPrinterFooter() const noexcept
            { return m_leftPrinterFooter; }

        /// @brief Sets the center printer footer string.
        /// @param footer The string to set as the center footer (when printed).
        void SetCenterPrinterFooter(const wxString& footer)
            { m_centerPrinterFooter = footer; }
        /// @returns The center printer footer string.
        [[nodiscard]] const wxString& GetCenterPrinterFooter() const noexcept
            { return m_centerPrinterFooter; }

        /// @brief Sets the right printer footer string.
        /// @param footer The string to set as the right footer (when printed).
        void SetRightPrinterFooter(const wxString& footer)
            { m_rightPrinterFooter = footer; }
        /// @returns The right printer footer string.
        [[nodiscard]] const wxString& GetRightPrinterFooter() const noexcept
            { return m_rightPrinterFooter; }
        /// @}

        /// @brief Calculates the sizes of all objects on the canvas.
        /// @param dc The DC to measure content with.
        /// @details Call this if customizations have been made to a subobject
        ///     (e.g., a plot) and you wish to refresh the content.
        void CalcAllSizes(wxDC& dc);
        /** @brief The scaling of the canvas's size compared to the default minimum size.
            @details This is used to see how much fonts and lines need to be increased
                to match the screen size.
            @returns The scaling.*/
        [[nodiscard]] double GetScaling() const
            {
            // aspect ratio is the same when resizing or zooming the window,
            // so using width or height is interchangable here
            return std::max<double>(
                safe_divide<double>(
                    GetCanvasRectDIPs().GetWidth(),
                    GetCanvasMinWidthDIPs()),
                1.0f);
            }

        /// @brief Saves the canvas as an image.
        /// @param filePath The file path of the image to save to.
        /// @param options The export options for the image.
        /// @returns @c true upon successful saving.
        bool Save(const wxFileName& filePath, const UI::ImageExportOptions& options);

        /// @brief Assign a menu as the right-click menu for the canvas.
        /// @param menu The menu to assign.
        /// @warning The canvas will take ownership of the menu and delete it upon destruction.
        void AssignContextMenu(wxMenu* menu) noexcept
            {
            delete m_menu;
            m_menu = menu;
            }

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
            wxColour m_color{ wxColour(255,0,0,125) };
            /// @brief The direction that the text is drawn.
            WatermarkDirection m_direction{ WatermarkDirection::Diagonal };
            };

        /** @brief Draws a watermark label across a canvas.
            @param dc The device context to draw on.
            @param drawingRect The rect within the DC to draw within.
            @param watermark The label to draw across the canvas.*/
        static void DrawWatermarkLabel(wxDC& dc, const wxRect drawingRect,
                                       const Watermark& watermark);
        /** @brief Draws a watermark logo on the corner of a canvas.
            @param dc The device context to draw on.*/
        void DrawWatermarkLogo(wxDC& dc);

        /// @private
        void SetBackgroundImage(wxBitmapBundle&& backgroundImage,
                                const uint8_t opacity = wxALPHA_OPAQUE) noexcept
            {
            m_bgImage = std::move(backgroundImage);
            m_bgOpacity = opacity;
            }
        /// @private
        void SetWatermarkLogo(wxBitmapBundle&& watermark, const wxSize sz) noexcept
            {
            m_watermarkImg = std::move(watermark);
            m_watermarkImgSizeDIPs = sz;
            }
        /// @private
        [[nodiscard]] const std::vector<GraphItems::Label>& GetTopTitles() const noexcept
            { return m_topTitles; }
        /// @private
        [[nodiscard]] const std::vector<GraphItems::Label>& GetBottomTitles() const noexcept
            { return m_bottomTitles; }
        /// @private
        [[nodiscard]] const std::vector<GraphItems::Label>& GetLeftTitles() const noexcept
            { return m_leftTitles; }
        /// @private
        [[nodiscard]] const std::vector<GraphItems::Label>& GetRightTitles() const noexcept
            { return m_rightTitles; }
        /// @private
        [[nodiscard]] const std::shared_ptr<GraphItems::GraphItemBase>
                          GetFixedObject(const size_t row, const size_t column) const;
    private:
        /// @returns Direct access to the print data.
        [[nodiscard]] wxPrintData& GetPrinterData() noexcept
            { return m_printData; }
        /// @returns The background image being drawn on the canvas.
        [[nodiscard]] wxBitmapBundle& GetBackgroundImage() noexcept
            { return m_bgImage; }
        /** @brief Draws the left titles.
            @returns How much of the left margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the left margin.*/
        [[nodiscard]] long CalcLeftTitles(wxDC& dc, const long spacingWidth);
        /** @brief Draws the right titles.
            @returns How much of the right margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the right margin.*/
        [[nodiscard]] long CalcRightTitles(wxDC& dc, const long spacingWidth);
        /** @brief Draws the top titles.
            @returns How much of the top margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the top margin.*/
        [[nodiscard]] long CalcTopTitles(wxDC& dc, const long spacingWidth);
        /** @brief Draws the bottom titles.
            @returns How much of the bottom margin of the plot those title take up.
            @param dc DC to measure with.
            @param spacingWidth How much padding should be used for the bottom margin.*/
        [[nodiscard]] long CalcBottomTitles(wxDC& dc, const long spacingWidth);

        /// @returns The top-level floating (i.e., not anchored) object on the canvas located at @c pt.
        /// @param pt The point to look at.
        [[nodiscard]] std::vector<std::shared_ptr<GraphItems::GraphItemBase>>::reverse_iterator
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
        ///     It should *not* be used with font point sizes because DPI scaling is handled by
        ///     the OS for those. Instead, font sizes should only be scaled to the canvas's scaling.
        [[nodiscard]] double ScaleToScreenAndCanvas(const double value, wxDC& dc) const noexcept
            { return value * GetScaling() * dc.FromDIP(1); }

        /// @returns The rectangle area of the canvas.
        [[nodiscard]] const wxRect& GetCanvasRectDIPs() const noexcept
            { return m_rectDIPs; }

        /// @returns The (scaled) rectangle area of the canvas.
        [[nodiscard]] wxRect GetCanvasRect(wxDC& dc) const noexcept
            {
            wxRect scaledRect(m_rectDIPs);
            scaledRect.SetSize(dc.FromDIP(m_rectDIPs.GetSize()));
            return scaledRect;
            }

        // Events
        void OnResize([[maybe_unused]] wxSizeEvent& event);
        void OnPaint([[maybe_unused]] wxPaintEvent& event);
        void OnContextMenu([[maybe_unused]] wxContextMenuEvent& event);
        void OnMouseEvent(wxMouseEvent& event);
        void OnKeyDown(wxKeyEvent& event);

        [[nodiscard]] auto& GetFixedObjects() noexcept
            { return m_fixedObjects; }
        [[nodiscard]] const auto& GetFixedObjects() const noexcept
            { return m_fixedObjects; }

        [[nodiscard]] std::vector<std::shared_ptr<GraphItems::Label>>& GetTitles() noexcept
            { return m_titles; }
        [[nodiscard]] const std::vector<std::shared_ptr<GraphItems::Label>>& GetTitles() const noexcept
            { return m_titles; }

        static constexpr double ZOOM_FACTOR{ 1.5 };
        int m_zoomLevel{ 0 };

        // the current drawing rect
        wxRect m_rectDIPs;
        // the minimum size of the canvas
        wxSize m_canvasMinSizeDIPs{ 0, 0 };
        static const int m_defaultWidthDIPs{ 700 };
        static const int m_defaultHeightDIPs{ 500 };

        bool m_alignRowContent{ false };
        bool m_alignColumnContent{ false };

        wxMenu* m_menu{ nullptr };
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
        std::vector<std::shared_ptr<GraphItems::Label>> m_titles;

        // embedded object (e.g., graphs, legends)
        std::vector<std::vector<std::shared_ptr<GraphItems::GraphItemBase>>> m_fixedObjects;
        std::vector<CanvasRowInfo> m_rowsInfo;

        // draggable items
        std::shared_ptr<wxDragImage> m_dragImage;
        std::vector<std::shared_ptr<GraphItems::GraphItemBase>> m_freeFloatingObjects;

        // watermarks and logos
        wxString m_watermark;
        wxFont m_watermarkFont;
        wxBitmapBundle m_watermarkImg;
        wxSize m_watermarkImgSizeDIPs{ 100, 100 };

        // background values
        wxColour m_bgColor{ *wxWHITE };
        uint8_t m_bgOpacity{ wxALPHA_OPAQUE };
        bool m_bgColorUseLinearGradient{ false };
        wxBitmapBundle m_bgImage;

        wxString m_debugInfo;
        };
    }

/** @}*/

#endif //__WISTERIA_CANVAS_H__
