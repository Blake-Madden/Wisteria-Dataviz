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
#include <cmath>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>
#include "graphitems.h"
#include "image.h"
#include "label.h"
#include "ui/imageexportdlg.h"
#include "ui/radioboxdlg.h"

DECLARE_EVENT_TYPE(EVT_WISTERIA_CANVAS_DCLICK, -1)

namespace Wisteria
    {
    /// @brief %Canvas for drawing, movable objects, etc.
    class Canvas : public wxScrolledWindow
        {
    public:
        friend class CanvasPrintout;

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
        Canvas(const Canvas&) = delete;
        Canvas(Canvas&&) = delete;
        Canvas& operator=(const Canvas&) = delete;
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
            @param opacity The opacity to render the image with.*/
        void SetBackgroundImage(GraphItems::Image& backgroundImage,
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
        /// @note The tags [DATETIME], [DATE], and [TIME] are expanded to their literal values at time of rendering.
        [[nodiscard]] wxString GetWatermark() const;
        /// @brief Overlays translucent image on bottom corner of the canvas.
        /// @param watermark The image to draw as a watermark (e.g., a company logo).
        void SetWatermarkLogo(const GraphItems::Image& watermark) noexcept
            {
            m_watermarkImg = watermark;
            m_watermarkImg.SetWindow(this);
            }
        /// @}

        /** @name Size Functions
            @brief Functions related to width and height measurements of the canvas.*/
        /// @{

        /// @returns The minimum width that the canvas can be, it will be forced to be this wide even as its parent is resized.
        [[nodiscard]] int GetCanvasMinWidth() const noexcept
            { return m_canvasMinWidth; }
        /** @brief Sets the minimum height that the canvas can be, it will be forced to be this tall even as its parent is resized.
            @param minWidth The minimum width to use.*/
        void SetCanvasMinWidth(const int minWidth) noexcept
           { m_canvasMinWidth = minWidth; }
        /// @returns The minimum height that the canvas can be, it will be forced to be this tall even as its parent is resized.
        [[nodiscard]] int GetCanvasMinHeight() const noexcept
           { return m_canvasMinHeight; }
        /** @brief Sets the minimum height that the canvas can be, it will be forced to be this tall even as its parent is resized.
            @param minHeight The minimum height to use.*/
        void SetCanvasMinHeight(const int minHeight) noexcept
           { m_canvasMinHeight = minHeight; }
        /// @returns The default minimum width used for canvas. Can be overridden by SetCanvasMinWidth().
        [[nodiscard]] int GetDefaultCanvasWidth() const
            { return FromDIP(wxSize(700,500)).GetWidth(); }
        /// @returns The default minimum height used for canvas. Can be overridden by SetCanvasMinHeight().
        [[nodiscard]] int GetDefaultCanvasHeight() const
            { return FromDIP(wxSize(700,500)).GetHeight(); }
        /// @returns The diagonal length of the canvas using the Pythagorean theorem.
        [[nodiscard]] long GetCanvasDiagonal() const
            {
            return static_cast<long>(std::sqrt(
                    (static_cast<double>(GetCanvasRect().GetWidth()) * GetCanvasRect().GetWidth()) +
                    (static_cast<double>(GetCanvasRect().GetHeight()) * GetCanvasRect().GetHeight())));
            }
        /** @brief Calculates the minimum percent of the canvas an items should consume when at 1.0 scaling.
            @param item The item to measure.
            @returns The percent of the canvas the item may need.
            @note This should be passed to the items SetCanvasWidthProportion() method prior to adding to the canvas.*/
        [[nodiscard]] double CalcMinWidthProportion(const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& item) const
            { return safe_divide<double>(item->GetBoundingBox().GetWidth(), GetCanvasMinWidth()); }
        /** @brief Calculates the minimum percent of the canvas an items should consume when at 1.0 scaling.
            @param item The item to measure.
            @returns The percent of the canvas the item may need.
            @note This can be passed to the canvas's SetRowProportion() method.*/
        [[nodiscard]] double CalcMinHeightProportion(const std::shared_ptr<Wisteria::GraphItems::GraphItemBase>& item) const
            { return safe_divide<double>(item->GetBoundingBox().GetHeight(), GetCanvasMinHeight()); }
        /// @}

        /** @name Layout Functions
            @brief Functions related to the fixed objects (e.g., plots) being embedded
             onto the canvas. These items are placed within a grid and accessed and managed through that.*/
        /// @{

        /** @brief Sets the number of rows and columns in the grid of fixed objects on the canvas.
            @param rows The number of rows to use.
            @param columns The number of columns to use.
            @note Contents currently in the grid will be preserved, unless @c rows or @c columns
             is smaller than the current size.
             Call ClearFixedObjects() if you need to clear the current contents.*/
        void SetFixedObjectsGridSize(const size_t rows, const size_t columns);
        /// @returns The size of the fixed object grid (number of rows x columns).
        [[nodiscard]] std::pair<size_t, size_t> GetFixedObjectsGridSize() const;
        /// @brief Removes all fixed objects and sets the grid back to (0,0).
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
        [[nodiscard]] std::shared_ptr<GraphItems::GraphItemBase> GetFixedObject(const size_t row, const size_t column);

        /** @brief Whether to align the content of items across each row.
            @details For example, this will set the Y axes of the plots
             (as well as stand alone axes) across a row to have the same height and positions.
            @param align `true` to align row contents.*/
        void AlignRowContent(const bool align) noexcept
            { m_alignRowContent = align; }
        /// @returns `true` if items (e.g., plots, common axes) are having their content
        ///  aligned with each other across each row.
        [[nodiscard]] bool IsRowContentAligned() const noexcept
            { return m_alignRowContent; }

        /** @brief Whether to align the content of items across each column.
            @details For example, this will set the X axes of the plots
             (as well as stand alone axes) down a column to have the same width and positions.
            @param align `true` to align column contents.
            @note If the grid is jagged, then the content alignment will stop on the first
             row that has less columns than the top row. For example, if a canvas has three rows,
             where the first and last rows have two graphs and the second only has one graph, then
             the X axes of the first column of graphs will be aligned, but not the second column.
             This is because the second row does not have a second column, so alignment adjusting
             stops at that point.*/
        void AlignColumnContent(const bool align) noexcept
            { m_alignColumnContent = align; }
        /// @returns `true` if items (e.g., plots, common axes) are having their content
        ///  aligned with each other down each column.
        [[nodiscard]] bool IsColumnContentAligned() const noexcept
            { return m_alignColumnContent; }

        /** @brief Sets the proportion of the canvas (height-wise) that the given row should consume.
            @param row The row index.
            @param proportion The percent of the canvas the row should consume.
            @warning When setting a row's proportion, be sure to reset all other row proportions
             as well; otherwise, they will not all add up to 100%. This is because when a grid size
             is specified, the rows are given uniform proportions. If one is changes, then the sum
             of all row proportions will be more or less than 100%. You must adjust all other rows
             accordingly.*/
        void SetRowProportion(const size_t row, const double proportion) noexcept
            {
            wxASSERT_MSG(row < m_rowProportions.size(), "Invalid row in call to SetRowProportion()!");
            if (row >= m_rowProportions.size())
                { return; }
            m_rowProportions.at(row) = proportion;
            }
        /// @}

        /** @name Free-floating objects Functions
            @brief Functions related to the floating objects (e.g., a label used as a "sticky note") being shown
             onto the canvas. These items are not part of the fixed object grid, but instead placed anywhere
             on the canvas, sitting on top of the grid.*/
        /// @{

        /** @brief Gets/sets the free floating (i.e., movable) objects on the canvas.
            @note These items are never cleared by the canvas itself and are not connected to the anything.
             When the canvas is resized, the size and position of these items does **not** change.
             Also, the canvas takes ownership of any objects added to this collection.
            @returns The free-floating objects.*/
        [[nodiscard]] std::vector<std::shared_ptr<GraphItems::GraphItemBase>>& GetFreeFloatingObjects() noexcept
            { return m_freeFloatingObjects; }
        /// @}

        /** @name Title Functions
            @brief Functions related to titles around the canvas.*/
        /// @{

        /// @returns The top titles. This can be used to add or edit a top title.
        /// @note Call Label::SetRelativeAlignment() to adjust the alignment of the title on the canvas.
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
            @brief Functions related to printing and printer settings.*/
        /// @{

        /** @brief Sets the printer data.
            @param printData The printer data (i.e., system print settings).*/
        void SetPrinterData(wxPrintData* printData) noexcept
            { m_printData = printData; }
        /// @returns The printer data.
        [[nodiscard]] wxPrintData& GetPrinterData() noexcept
            { return *m_printData; }

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
        /// @details Call this if customizations have been made to a subobject (e.g., a plot) and you
        ///  wish to refresh the content.
        void CalcAllSizes();
        /** @brief The scaling of the size of the canvas compared to the default minimum size.
            @details This is used to see how much fonts and lines need to be increased to match the screen size.
            @returns The scaling.*/
        [[nodiscard]] double GetScaling() const
            { return std::max<double>(safe_divide<double>(GetCanvasRect().GetWidth(), GetCanvasMinWidth()), 1.0f); }

        /// @brief Saves the canvas as an image.
        /// @param filePath The file path of the image to save to.
        /// @param options The export options for the image.
        /// @returns `true` upon successful saving.
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
            @details These are set by the parent application to connect icons and help topics to the export dialog.
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
        struct WaterMark
            {
            wxString m_label;                                               /*!< The text.*/
            wxColour m_color{ wxColour(255,0,0,125) };                      /*!< The text color.*/
            WatermarkDirection m_direction{ WatermarkDirection::Diagonal }; /*!< The direction that the text is drawn.*/
            };

        /** @brief Draws a watermark label across a canvas.
            @param dc The device context to draw on.
            @param drawingRect The rect within the DC to draw within.
            @param watermark The label to draw across the canvas.*/
        static void DrawWatermarkLabel(wxDC& dc, const wxRect drawingRect, const WaterMark& watermark);
        /** @brief Draws a watermark logo on the corner of a canvas.
            @param dc The device context to draw on.*/
        void DrawWatermarkLogo(wxDC& dc);

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
        [[nodiscard]] const std::shared_ptr<GraphItems::GraphItemBase> GetFixedObject(const size_t row, const size_t column) const;
    private:
        /// @returns The background image being drawn on the canvas.
        [[nodiscard]] GraphItems::Image& GetBackgroundImage() noexcept
            { return m_bgImage; }
        /** @brief Draws the left titles.
            @returns How much of the left margin of the plot those title take up.
            @param measureDC DC to measure with.
            @param spacingWidth How much padding should be used for the left margin.*/
        [[nodiscard]] long CalcLeftTitles(wxDC& measureDC, const long spacingWidth);
        /** @brief Draws the right titles.
            @returns How much of the right margin of the plot those title take up.
            @param measureDC DC to measure with.
            @param spacingWidth How much padding should be used for the right margin.*/
        [[nodiscard]] long CalcRightTitles(wxDC& measureDC, const long spacingWidth);
        /** @brief Draws the top titles.
            @returns How much of the top margin of the plot those title take up.
            @param measureDC DC to measure with.
            @param spacingWidth How much padding should be used for the top margin.*/
        [[nodiscard]] long CalcTopTitles(wxDC& measureDC, const long spacingWidth);
        /** @brief Draws the bottom titles.
            @returns How much of the bottom margin of the plot those title take up.
            @param measureDC DC to measure with.
            @param spacingWidth How much padding should be used for the bottom margin.*/
        [[nodiscard]] long CalcBottomTitles(wxDC& measureDC, const long spacingWidth);

        /// @returns The top-level floating (i.e., not anchored) object on the canvas located at @c pt.
        /// @param pt The point to look at.
        [[nodiscard]] std::vector<std::shared_ptr<GraphItems::GraphItemBase>>::reverse_iterator FindFreeFloatingObject(const wxPoint& pt);

        enum class DragMode
            {
            DraggingNone,
            DragStart,
            Dragging
            };

        /// @brief Apply screen DPI and parent canvas scaling to a value.
        /// @param value The value (e.g., pen width) to scale.
        /// @returns The scaled value.
        /// @note This should be used to rescale pixel values used for line widths and point sizes.
        ///  It should NOT be used with font point sizes because DPI scaling handled by the OS for those.
        ///  Instead, font sizes should only be scaled to the canvas's scaling.
        [[nodiscard]] double ScaleToScreenAndCanvas(const double value) const noexcept
            { return value*GetScaling()*GetDPIScaleFactor(); }

        /// @returns The rectangle area of the canvas.
        [[nodiscard]] const wxRect& GetCanvasRect() const noexcept
            { return m_rect; }

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

        wxRect m_rect;
        // The minimum width of the canvas.
        int m_canvasMinWidth{ 0 };
        // The minimum height of the canvas.
        int m_canvasMinHeight{ 0 };

        bool m_alignRowContent{ false };
        bool m_alignColumnContent{ false };

        wxMenu* m_menu{ nullptr };
        wxPrintData* m_printData{ nullptr };
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
        std::vector<double> m_rowProportions;

        // draggable items
        std::shared_ptr<wxDragImage> m_dragImage;
        std::vector<std::shared_ptr<GraphItems::GraphItemBase>> m_freeFloatingObjects;

        // watermarks and logos
        wxString m_watermark;
        GraphItems::Image m_watermarkImg;
        wxFont m_watermarkFont;

        // background values
        wxColour m_bgColor{ *wxWHITE };
        uint8_t m_bgOpacity{ wxALPHA_OPAQUE };
        bool m_bgColorUseLinearGradient{ false };
        Wisteria::GraphItems::Image m_bgImage;
        };
    }

/** @}*/

#endif //__WISTERIA_CANVAS_H__
