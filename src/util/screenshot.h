/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __SCREENSHOT_HELPER_H__
#define __SCREENSHOT_HELPER_H__

#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/listctrl.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/filename.h>
#include "../math/mathematics.h"

/// @brief Class for creating and editing screenshots.
class Screenshot
    {
public:
    /** @brief Saves a screenshot of the active window.
        @param filePath The path to save the screenshot to.
        @param startIdToHighlight The (optional) start control to draw a red line around.
        @param endIdToHighlight The (optional) end control to draw a red line around.
        @param cutoffId An (optional) ID it cutoff vertically at.
            (This will be the last control at the bottom of the screenshot.)
        @returns @c true if image is saved successfully.*/
    static bool SaveScreenshot(const wxString& filePath,
                               const wxWindowID startIdToHighlight = wxID_ANY,
                               const wxWindowID endIdToHighlight = wxID_ANY,
                               const wxWindowID cutoffId = wxID_ANY);
    /** @brief Saves a screenshot of the active window, and an annotation written over provided control(s).
        @param filePath The path to save the screenshot to.
        @param annotation Text to write in the provided area.
        @param startIdToOverwrite The start control to draw the annotation over.
        @param endIdToOverwrite The start control to draw the annotation over.
        @returns @c true if image is saved successfully.*/
    static bool SaveScreenshot(const wxString& filePath,
                               const wxString& annotation,
                               const wxWindowID startIdToOverwrite,
                               const wxWindowID endIdToOverwrite = wxID_ANY);
    /** @brief Saves a screenshot of the active window and highlights items in its property grid.
        @param filePath The path to save the screenshot to.
        @param propertyGridId The window ID of the property grid.
            If provided, will search for the top-most property grid with that ID.
            If @c wxID_ANY, then top-most property grid found will be used
        @param startIdToHighlight The (optional) starting grid row to draw a red line around.
        @param endIdToHighlight The (optional) ending grid row to draw a red line around.
        @param cropToGridHeight Whether to crop the screenshot to the height of the
            property grid's content.
        @returns @c true if image is saved successfully.*/
    static bool SaveScreenshotOfDialogWithPropertyGrid(const wxString& filePath,
                                                       const wxWindowID propertyGridId = wxID_ANY,
                                                       const wxString& startIdToHighlight = wxEmptyString,
                                                       wxString endIdToHighlight = wxEmptyString,
                                                       const bool cropToGridHeight = false);
    /** @brief Saves a screenshot of a text window.
        @param filePath The path to save the screenshot to.
        @param windowId The ID of the text window. The window will be searched for from the
            top-level window by looking for this ID.
        @param clipContents Whether empty area at the bottom of the window should be
            clipped from the image.
        @param highlightPoints The pairs of character positions to draw a red highlight
            around in the screenshot.
        @returns @c true if image is saved successfully.*/
    static bool SaveScreenshotOfTextWindow(const wxString& filePath,
                        const wxWindowID windowId, const bool clipContents,
                        const std::vector<std::pair<long,long>>& highlightPoints);

    /** @brief Saves a screenshot of the @c wxListCtrl.
        @param filePath The path to save the screenshot to.
        @param windowId The ID of the window. The window will be searched for from the
            top-level window by looking for this ID.
        @param startRow The first row of the list control to scroll to.
        @param endRow The last row of the list control to scroll to.
        @param startColumn The first column of the list control to ensure is visible.
        @param endColumn The last column of the list control to ensure is visible.
        @param cuttOffRow First row to chop off in the screenshot.
        @returns @c true if image is saved successfully.*/
    static bool SaveScreenshotOfListControl(const wxString& filePath,
                        const wxWindowID windowId,
                        const long startRow = -1, const long endRow = -1,
                        const long startColumn = -1, const long endColumn = -1,
                        const long cuttOffRow = -1);

    /** @brief Saves a screenshot of the top-level ribbon.
        @param filePath The path to save the screenshot to.
        @param pageToSelect The ribbon page to select.
        @param buttonBarToHighlight The button bar area (on the active page) to highlight.
        @note Unfortunately, highlighting an individual button doesn't seem possible
            because the buttons are drawn dynamically and aren't parented by the
            button bar or ribbon itself.
        @returns @c true if image is saved successfully.*/
    static bool SaveScreenshotOfRibbon(const wxString& filePath,
                        const int pageToSelect = 0,
                        const wxWindowID buttonBarToHighlight = wxID_ANY);

    /** @brief Opens a screenshot and draws a read square at given coordinates.
        @param filePath The path of the image to open.
        @param topLeftCorner The top-left corner of the rect to highlight.
        @param bottomRightCorner The bottom-right corner of the rect to highlight.
        @returns @c true if image is saved successfully.
        @warning The file needs to be a bitmap.*/
    static bool HighlightItemInScreenshot(const wxString& filePath,
                                          const wxPoint topLeftCorner,
                                          const wxPoint bottomRightCorner);

    /** @brief Opens a screenshot and draws a text box at the provided area.
        @param filePath The path of the image to open.
        @param text The text to write in the box.
        @param topLeftCorner The top-left corner of the text box.
        @param bottomRightCorner The bottom-right corner of the text box.
        @returns @c true if image is saved successfully.
        @warning The file needs to be a bitmap.*/
    static bool AnnotateScreenshot(const wxString& filePath,
                                   const wxString& text,
                                   const wxPoint topLeftCorner,
                                   const wxPoint bottomRightCorner);

    /** @brief Opens a screenshot and crops to the provided dimensions.
        @param filePath The path of the image to open.
        @param width The width to crop the image to. Leave as @c wxDefaultCoord to not crop the width.
        @param height The width to crop the image to. Leave as @c wxDefaultCoord to not crop the height.
        @returns @c true if image is saved successfully.
        @warning The file needs to be a bitmap.*/
    static bool CropScreenshot(const wxString& filePath,
                               wxCoord width = wxDefaultCoord, wxCoord height = wxDefaultCoord);

    /** @brief Converts an image to a PNG file (and downscaling it as necessary).
        @param filePath The image to convert.
        @param scaledSize The (smaller) size to convert the image to.\n
            Note that this size is a request, as aspect ratio will be preserved.
        @param removeOriginalFile @c true to delete the original image.
        @returns @c true if the image was successfully converted and saved.*/
    static bool ConvertImageToPng(const wxString& filePath, const wxSize scaledSize,
        const bool removeOriginalFile = false);

    /// @returns The active dialog or frame.
    /// @note @c wxGetActiveWindow() always returns null on macOS, so this
    ///     uses @c wxWindow::FindFocus() and moves up to the parent dialog or frame.
    ///     If that doesn't work, then falls back to @c wxGetActiveWindow().
    ///     In that case, may return null.
    /// @warning This is only meant for screenshots. Use @c wxTheApp->GetTopWindow()
    ///     to get the app's main window.
    [[nodiscard]]
    static wxWindow* GetActiveDialogOrFrame();
private:
    /// @brief Ensures that everything is repainted and ready for screenshot.
    /// @param windowToCapture The window to prepare for the screenshot.
    static void PrepareWindowForScreenshot(wxWindow* windowToCapture);
    static void AddBorderToImage(wxBitmap& bmp);

    static wxPen GetScreenshotHighlightPen(const int width)
        { return wxPen(*wxRED, width, wxPenStyle::wxPENSTYLE_DOT); }
    static wxPen GetOutlintPen(const int width)
        { return wxPen(*wxLIGHT_GREY, width); }
    };

/** @}*/

#endif //__SCREENSHOT_HELPER_H__
