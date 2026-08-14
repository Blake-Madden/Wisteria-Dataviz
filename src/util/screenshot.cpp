///////////////////////////////////////////////////////////////////////////////
// Name:        screenshot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "screenshot.h"
#include "../math/mathematics.h"
#include <array>
#include <wx/buffer.h>
#include <wx/listctrl.h>
#include <wx/mstream.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/tokenzr.h>
#include <wx/webview.h>
#if defined(__WXMSW__) && wxUSE_WEBVIEW_EDGE
    #include <wx/msw/private.h>
    #include <wx/msw/private/comptr.h>
    #ifdef __VISUALC__
        #include <wrl/event.h>
using namespace Microsoft::WRL;
    #else
        #include <wx/msw/wrl/event.h>
    #endif
    #include <WebView2.h>
    #include <objbase.h> // CreateStreamOnHGlobal()
#endif

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
//---------------------------------------------------
bool Screenshot::ConvertImageToPng(const wxString& filePath, const wxSize scaledSize,
                                   const bool removeOriginalFile /*= false*/)
    {
    const wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        wxFileName fn(filePath);
        fn.SetExt(L"png");
        wxImage img(bmp.ConvertToImage());
        const auto [newWidth, newHeight] = geometry::downscaled_size(
            std::make_pair(img.GetWidth(), img.GetHeight()),
            std::make_pair(scaledSize.GetWidth(), scaledSize.GetHeight()));
        if (!img.Rescale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH)
                 .SaveFile(fn.GetFullPath(), wxBitmapType::wxBITMAP_TYPE_PNG))
            {
            wxLogWarning(L"Unable to save '%s' when converting screenshot.", fn.GetFullPath());
            return false;
            }
        if (removeOriginalFile)
            {
            if (!wxRemoveFile(filePath))
                {
                wxLogWarning(L"Unable to delete '%s' when converting screenshot.", filePath);
                }
            }
        return true;
        }

    return false;
    }

//---------------------------------------------------
bool Screenshot::HighlightItemInScreenshot(const wxString& filePath, const wxPoint topLeftCorner,
                                           const wxPoint bottomRightCorner)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        wxMemoryDC memDC;
        memDC.SelectObject(bmp);
        memDC.SetPen(GetScreenshotHighlightPen(wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
        memDC.DrawLine(topLeftCorner.x, topLeftCorner.y, bottomRightCorner.x, topLeftCorner.y);
        memDC.DrawLine(bottomRightCorner.x, topLeftCorner.y, bottomRightCorner.x,
                       bottomRightCorner.y);
        memDC.DrawLine(bottomRightCorner.x, bottomRightCorner.y, topLeftCorner.x,
                       bottomRightCorner.y);
        memDC.DrawLine(topLeftCorner.x, bottomRightCorner.y, topLeftCorner.x, topLeftCorner.y);
        memDC.SelectObject(wxNullBitmap);

        return bmp.SaveFile(filePath, wxBitmapType::wxBITMAP_TYPE_BMP);
        }

    return false;
    }

//---------------------------------------------------
bool Screenshot::AnnotateScreenshot(const wxString& filePath, const wxString& text,
                                    const wxPoint topLeftCorner, const wxPoint bottomRightCorner)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        wxMemoryDC memDC;
        memDC.SelectObject(bmp);
        memDC.SetPen(GetOutlinePen(wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
        memDC.SetBrush(wxColour{ 255, 255, 255 });
        memDC.DrawRectangle(wxRect{ topLeftCorner, bottomRightCorner });
        memDC.DrawText(text, topLeftCorner);

        memDC.SelectObject(wxNullBitmap);

        return bmp.SaveFile(filePath, wxBitmapType::wxBITMAP_TYPE_BMP);
        }

    return false;
    }

//---------------------------------------------------
bool Screenshot::CropScreenshot(const wxString& filePath, wxCoord width, wxCoord height)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        if (width == wxDefaultCoord)
            {
            width = bmp.GetWidth();
            }
        if (height == wxDefaultCoord)
            {
            height = bmp.GetHeight();
            }
        bmp = bmp.GetSubBitmap(wxRect{ 0, 0, width, height });

        AddBorderToImage(bmp);

        return bmp.SaveFile(filePath, wxBitmapType::wxBITMAP_TYPE_BMP);
        }

    return false;
    }

//---------------------------------------------------
void Screenshot::AddBorderToImage(wxBitmap& bmp) // cppcheck-suppress constParameter
    {
    wxMemoryDC memDC;
    memDC.SelectObject(bmp);

    memDC.SetPen(wxPen(wxColour{ 241, 241, 241 }, wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
    const std::array<wxPoint, 5> corners = {
        wxPoint(0, 0), wxPoint(memDC.GetSize().GetWidth() - memDC.GetPen().GetWidth(), 0),
        wxPoint(memDC.GetSize().GetWidth() - memDC.GetPen().GetWidth(),
                memDC.GetSize().GetHeight() - memDC.GetPen().GetWidth()),
        wxPoint(0, memDC.GetSize().GetHeight() - memDC.GetPen().GetWidth()), wxPoint(0, 0)
    };
    memDC.DrawLines(corners.size(), corners.data());
    memDC.SelectObject(wxNullBitmap);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfRibbon(const wxString& filePath, const int pageToSelect /*= 0*/,
                                        const wxWindowID firstButtonBarToHighlight /*= wxID_ANY*/,
                                        const wxWindowID lastButtonBarToHighlight /*= wxID_ANY*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }

    wxWindow* foundWindow{ nullptr };
    if (!windowToCapture->IsKindOf(CLASSINFO(wxRibbonBar)))
        {
        auto& children = windowToCapture->GetChildren();
        for (const auto& child : children)
            {
            if ((child != nullptr) && child->IsKindOf(CLASSINFO(wxRibbonBar)))
                {
                foundWindow = child;
                break;
                }
            }
        }
    if (foundWindow == nullptr)
        {
        return false;
        }

    auto* ribbonBar = dynamic_cast<wxRibbonBar*>(foundWindow);
    wxASSERT(ribbonBar);

    if (!ribbonBar->SetActivePage(pageToSelect))
        {
        return false;
        }
    wxRibbonPage* activePage{ ribbonBar->GetPage(pageToSelect) };
    const size_t panelCount{ activePage->GetPanelCount() };
    wxTheApp->Yield();

    wxClientDC dc(ribbonBar);
    wxMemoryDC memDC;

    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    const wxWindow* lastButtonBar = [&]() -> wxWindow*
    {
        if (lastButtonBarToHighlight != wxID_ANY)
            {
            for (size_t i = 0; i < panelCount; ++i)
                {
                const wxRibbonPanel* currentPanel{ activePage->GetPanel(i) };
                if (currentPanel != nullptr && currentPanel->IsShown())
                    {
                    auto* buttonBar = currentPanel->FindWindow(lastButtonBarToHighlight);
                    return (buttonBar != nullptr &&
                            buttonBar->IsKindOf(CLASSINFO(wxRibbonButtonBar))) ?
                               buttonBar :
                               nullptr;
                    }
                }
            return nullptr;
            }

        return nullptr;
    }();

    if (firstButtonBarToHighlight != wxID_ANY)
        {
        for (size_t i = 0; i < panelCount; ++i)
            {
            const wxRibbonPanel* currentPanel{ activePage->GetPanel(i) };
            if (currentPanel != nullptr && currentPanel->IsShown())
                {
                const auto* buttonBar = currentPanel->FindWindow(firstButtonBarToHighlight);
                if ((buttonBar != nullptr) && buttonBar->IsKindOf(CLASSINFO(wxRibbonButtonBar)))
                    {
                    /* Step back all the way from the child window to the parent and tally the
                       offset of the children relative to its parent. When dealing with client
                       areas, using the screen position of controls will be off because the main
                       dialog's decorations aren't factored into that.*/
                    wxPoint startPoint{ 0, 0 };
                    const auto* startWindowParent = buttonBar;
                    while (startWindowParent != nullptr && startWindowParent != ribbonBar)
                        {
                        startPoint += startWindowParent->GetPosition();
                        startWindowParent = startWindowParent->GetParent();
                        }
                    const wxSize lastButtonBarSize =
                        (lastButtonBar != nullptr) ? lastButtonBar->GetSize() : wxSize{};
                    wxPoint endPoint(startPoint.x + buttonBar->GetSize().GetWidth() +
                                         lastButtonBarSize.GetWidth(),
                                     startPoint.y + buttonBar->GetSize().GetHeight());
                    // add a little padding around the control(s) being highlighted
                    startPoint -= wxPoint{ wxSizerFlags::GetDefaultBorder(),
                                           wxSizerFlags::GetDefaultBorder() };
                    // adjust if outside render area
                    startPoint.x = std::max(
                        startPoint.x, static_cast<int>(windowToCapture->GetDPIScaleFactor()) * 2);
                    startPoint.y = std::max(
                        startPoint.y, static_cast<int>(windowToCapture->GetDPIScaleFactor()) * 2);

                    endPoint +=
                        wxPoint{ // same for end point, but make sure we didn't go off the screen
                                 (endPoint.x + wxSizerFlags::GetDefaultBorder() <
                                  memDC.GetSize().GetWidth()) ?
                                     wxSizerFlags::GetDefaultBorder() :
                                     0,
                                 (endPoint.y + wxSizerFlags::GetDefaultBorder() <
                                  memDC.GetSize().GetHeight()) ?
                                     wxSizerFlags::GetDefaultBorder() :
                                     0
                        };
                    memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
                    memDC.DrawLine(startPoint.x, startPoint.y, endPoint.x, startPoint.y);
                    memDC.DrawLine(endPoint.x, startPoint.y, endPoint.x, endPoint.y);
                    memDC.DrawLine(endPoint.x, endPoint.y, startPoint.x, endPoint.y);
                    memDC.DrawLine(startPoint.x, endPoint.y, startPoint.x, startPoint.y);
                    }
                }
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfListControl(const wxString& filePath, const wxWindowID windowId,
                                             const long startRow /*= -1*/,
                                             const long endRow /*= -1*/,
                                             const long startColumn /*= -1*/,
                                             const long endColumn /*= -1*/,
                                             const long cutOffRow /*= -1*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }
    if (windowToCapture->GetId() != windowId || !windowToCapture->IsKindOf(CLASSINFO(wxListCtrl)))
        {
        wxWindow* foundWindow = windowToCapture->FindWindow(windowId);
        if (foundWindow != nullptr && foundWindow->IsKindOf(CLASSINFO(wxListCtrl)))
            {
            windowToCapture = foundWindow;
            }
        else
            {
            return false;
            }
        }
    auto* listCtrl = dynamic_cast<wxListCtrl*>(windowToCapture);
    wxASSERT_MSG(listCtrl, L"Invalid list control for screenshot!");
    if (listCtrl == nullptr)
        {
        return false;
        }

    long columnsWidth{ 0 };
    for (auto i = 0; i < listCtrl->GetColumnCount(); ++i)
        {
        columnsWidth += listCtrl->GetColumnWidth(i);
        }
    long rowHeight{ 0 };
    if (listCtrl->GetItemCount() != 0)
        {
        wxRect itemRect;
        listCtrl->GetItemRect(0, itemRect);
        rowHeight = itemRect.GetHeight() * (listCtrl->GetItemCount() + 1.5 /*header*/);
        }

    if (endRow != -1)
        {
        listCtrl->EnsureVisible(endRow);
        listCtrl->EnsureVisible(startRow);
        wxTheApp->Yield();
        }
    if (cutOffRow != -1)
        {
        listCtrl->EnsureVisible(cutOffRow);
        wxTheApp->Yield();
        }

    wxClientDC dc(listCtrl);
    wxMemoryDC memDC;

    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (startRow != -1 || endRow != -1 || startColumn != -1 || endColumn != -1)
        {
        wxRect startRect, endRect;
        if (listCtrl->GetSubItemRect((startRow == -1 ? 0 : startRow),
                                     (startColumn == -1 ? 0 : startColumn), startRect) &&
            listCtrl->GetSubItemRect((endRow == -1 ? listCtrl->GetItemCount() - 1 : endRow),
                                     (endColumn == -1 ? listCtrl->GetColumnCount() - 1 : endColumn),
                                     endRect))
            {
            wxRect highlightRect(startRect.GetTopLeft(), endRect.GetBottomRight());
            highlightRect.x += listCtrl->GetScrollPos(wxHORIZONTAL);
            highlightRect.y += listCtrl->GetScrollPos(wxVERTICAL);
            const wxDCPenChanger pc(
                memDC, GetScreenshotHighlightPen(wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
            const wxDCBrushChanger bc(memDC, *wxTRANSPARENT_BRUSH);
            memDC.DrawRectangle(highlightRect);
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // chop off rows that user doesn't want included
    if (cutOffRow != -1 && cutOffRow + 1 < listCtrl->GetItemCount())
        {
        wxRect cutOffRect;
        if (listCtrl->GetSubItemRect(
                // get the top of the row below the cut-off
                cutOffRow + 1, 0, cutOffRect))
            {
            bitmap = bitmap.GetSubBitmap(wxRect{ 0, 0, bitmap.GetWidth(), cutOffRect.GetTop() });
            }
        }
    // chop off any dead space after last column
    if (columnsWidth < bitmap.GetWidth())
        {
        bitmap = bitmap.GetSubBitmap(wxRect(0, 0,
                                            columnsWidth +
                                                // space for the pen if we are right on the edge
                                                wxTheApp->GetTopWindow()->GetDPIScaleFactor(),
                                            bitmap.GetHeight()));
        }
    // and below the last row
    // (this assumes there are fewer rows in the entire list that fix on the screen)
    if (rowHeight < bitmap.GetHeight())
        {
        bitmap = bitmap.GetSubBitmap(wxRect(
            0, 0, bitmap.GetWidth(), rowHeight + wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfTextWindow(
    const wxString& filePath, const wxWindowID windowId, const bool clipContents,
    const std::vector<std::pair<long, long>>& highlightPoints)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }
    if (windowToCapture->GetId() != windowId || !windowToCapture->IsKindOf(CLASSINFO(wxTextCtrl)))
        {
        wxWindow* foundWindow = windowToCapture->FindWindow(windowId);
        if (foundWindow != nullptr && foundWindow->IsKindOf(CLASSINFO(wxTextCtrl)))
            {
            windowToCapture = foundWindow;
            }
        else
            {
            return false;
            }
        }

    PrepareWindowForScreenshot(windowToCapture);

    if (!highlightPoints.empty())
        {
        dynamic_cast<wxTextCtrl*>(windowToCapture)->ShowPosition(highlightPoints[0].first);
        // give UI time to scroll and refresh
        ::wxSleep(2);
        }

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    const wxTextCtrl* textWindow = dynamic_cast<wxTextCtrl*>(windowToCapture);

    for (const auto& highlightPoint : highlightPoints)
        {
        if (textWindow != nullptr)
            {
            wxPoint startPoint = textWindow->PositionToCoords(highlightPoint.first);
            wxPoint endPoint = (highlightPoint.second != -1) ?
                                   textWindow->PositionToCoords(highlightPoint.second) :
                                   textWindow->PositionToCoords(textWindow->GetLastPosition());
            // if points are on different lines, then highlight the whole row
            if (startPoint.y != endPoint.y)
                {
                startPoint.x = 0;
                endPoint.x = memDC.GetSize().GetWidth() - memDC.GetPen().GetWidth();
                }
            long x{ 0 }, y{ 0 };
            if ((highlightPoint.second != -1) &&
                textWindow->PositionToXY(highlightPoint.second, &x, &y))
                {
                endPoint.y = textWindow->PositionToCoords(textWindow->XYToPosition(0, y + 1)).y;
                }
            else
                {
                endPoint.y += (textWindow->GetDefaultStyle().GetFontSize() * 2);
                }
            // adjust in case the lines are on the edge of the DC
            startPoint.x = std::max(startPoint.x, 1);
            startPoint.y = std::max(startPoint.y, 1);

            endPoint.x = std::min(endPoint.x,
                                  memDC.GetSize().GetWidth() -
                                      static_cast<int>(windowToCapture->GetDPIScaleFactor() + 1));
            endPoint.y = std::min(endPoint.y,
                                  memDC.GetSize().GetHeight() -
                                      static_cast<int>(windowToCapture->GetDPIScaleFactor() + 1));

            memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
            memDC.DrawLine(startPoint.x, startPoint.y, endPoint.x, startPoint.y);
            memDC.DrawLine(endPoint.x, startPoint.y, endPoint.x, endPoint.y);
            memDC.DrawLine(endPoint.x, endPoint.y, startPoint.x, endPoint.y);
            memDC.DrawLine(startPoint.x, endPoint.y, startPoint.x, startPoint.y);
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // always clip dead space at the bottom
    if (textWindow != nullptr)
        {
        wxPoint endOfWindowPoint = textWindow->PositionToCoords(textWindow->GetLastPosition());
        endOfWindowPoint.y += (textWindow->GetDefaultStyle().GetFontSize() * 2);
        if (endOfWindowPoint.y < bitmap.GetHeight())
            {
            bitmap = bitmap.GetSubBitmap(wxRect{ 0, 0, bitmap.GetWidth(), endOfWindowPoint.y });
            }
        }

    // if clipping to highlights, additionally crop below the last highlighted section
    if (clipContents && !highlightPoints.empty() && textWindow != nullptr)
        {
        const auto& lastHighlight = highlightPoints.back();
        wxPoint endPoint = (lastHighlight.second != -1) ?
                               textWindow->PositionToCoords(lastHighlight.second) :
                               textWindow->PositionToCoords(textWindow->GetLastPosition());
        long x{ 0 }, y{ 0 };
        if ((lastHighlight.second != -1) && textWindow->PositionToXY(lastHighlight.second, &x, &y))
            {
            endPoint.y = textWindow->PositionToCoords(textWindow->XYToPosition(0, y + 1)).y;
            }
        else
            {
            endPoint.y += (textWindow->GetDefaultStyle().GetFontSize() * 2);
            }
        if (endPoint.y < bitmap.GetHeight())
            {
            bitmap = bitmap.GetSubBitmap(wxRect{ 0, 0, bitmap.GetWidth(), endPoint.y });
            }
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfWebView(const wxString& filePath, const wxWindowID windowId,
                                         const bool clipContents,
                                         const std::vector<std::pair<long, long>>& highlightPoints)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }
    if (windowToCapture->GetId() != windowId || !windowToCapture->IsKindOf(CLASSINFO(wxWebView)))
        {
        wxWindow* foundWindow = windowToCapture->FindWindow(windowId);
        if (foundWindow != nullptr && foundWindow->IsKindOf(CLASSINFO(wxWebView)))
            {
            windowToCapture = foundWindow;
            }
        else
            {
            return false;
            }
        }

    auto* webView = dynamic_cast<wxWebView*>(windowToCapture);
    if (webView == nullptr)
        {
        return false;
        }

    PrepareWindowForScreenshot(windowToCapture);

    // The web view reports its layout in CSS pixels, but the DC we're drawing into
    // is in device pixels. Everything pulled out of a script below needs this to
    // land in the right place.
    const double dpiScale{ windowToCapture->GetDPIScaleFactor() };

    // The length of the same text-node walk that buildRangeScript() below does, used to
    // resolve an "end" of -1 (meaning "through the end of the content"). Note this is
    // *not* the same as wxWebView::GetPageText().length(), which is a differently
    // normalized (and thus differently indexed) rendering of the page's text.
    long totalTextLength{ 0 };
    wxString lengthOutput;
    if (webView->RunScript(L"(function() {" + GetWebViewTextWalkerScript() + LR"JS(
                               var len = 0, node;
                               while ((node = walker.nextNode())) { len += node.length; }
                               return len.toString();
                               )JS" +
                               L"})();",
                           &lengthOutput))
        {
        lengthOutput.ToLong(&totalTextLength);
        }

    // Builds a script that walks the page's text nodes and leaves a Range (named "range",
    // with "started" set to true if it was resolved) spanning the given character positions;
    // The caller appends the action to perform with that range.
    const auto buildRangeScript = [totalTextLength](const long start, const long end)
    {
        return GetWebViewTextWalkerScript() + wxString::Format(LR"JS(
                   var start = %ld, end = %ld;
                   var pos = 0, node, range = document.createRange(), started = false;
                   while ((node = walker.nextNode()))
                       {
                       var nodeStart = pos, nodeEnd = pos + node.length;
                       if (!started && end >= nodeStart && start < nodeEnd)
                           {
                           range.setStart(node, Math.max(0, start - nodeStart));
                           started = true;
                           }
                       if (started && end <= nodeEnd)
                           {
                           range.setEnd(node, Math.max(0, end - nodeStart));
                           break;
                           }
                       pos = nodeEnd;
                       }
                   )JS",
                                                               start,
                                                               (end == -1 ? totalTextLength : end));
    };

    if (!highlightPoints.empty())
        {
        const wxString scrollScript =
            L"(function() {" +
            buildRangeScript(highlightPoints[0].first, highlightPoints[0].first) + LR"JS(
            if (!started) { return '0'; }
            var r = range.getBoundingClientRect();
            window.scrollTo(0, Math.max(0, r.top + window.pageYOffset - 40));
            return '1';
            })();
            )JS";
        webView->RunScript(scrollScript);
        // give UI time to scroll and refresh
        ::wxSleep(2);
        }

    wxBitmap bitmap{ CaptureWebViewContent(webView) };

    wxMemoryDC memDC;
    memDC.SelectObject(bitmap);

    wxCoord lastHighlightBottom{ -1 };

    for (const auto& highlightPoint : highlightPoints)
        {
        // a single bounding box for the whole range, mirroring the one-box-per-highlight
        // behavior of SaveScreenshotOfTextWindow (rather than one box per line/element
        // fragment, which is what Range::getClientRects() would give us)
        const wxString rectScript = L"(function() {" +
                                    buildRangeScript(highlightPoint.first, highlightPoint.second) +
                                    LR"JS(
            if (!started) { return ''; }
            var r = range.getBoundingClientRect();
            return r.left + ',' + r.top + ',' + r.right + ',' + r.bottom;
            })();
            )JS";

        wxString scriptOutput;
        if (webView->RunScript(rectScript, &scriptOutput) && !scriptOutput.empty())
            {
            wxStringTokenizer coordTkz(scriptOutput, L",", wxTOKEN_STRTOK);
            std::array<double, 4> coords{ 0, 0, 0, 0 };
            size_t coordIndex{ 0 };
            while (coordTkz.HasMoreTokens() && coordIndex < coords.size())
                {
                if (!coordTkz.GetNextToken().ToDouble(&coords[coordIndex]))
                    {
                    break;
                    }
                ++coordIndex;
                }
            if (coordIndex == coords.size())
                {
                const wxPoint startPoint{ static_cast<int>(coords[0] * dpiScale),
                                          static_cast<int>(coords[1] * dpiScale) };
                const wxPoint endPoint{ static_cast<int>(coords[2] * dpiScale),
                                        static_cast<int>(coords[3] * dpiScale) };
                const int highlightPenWidth{ std::max(
                    1, static_cast<int>(windowToCapture->GetDPIScaleFactor())) };
                memDC.SetPen(GetScreenshotHighlightPen(highlightPenWidth));
                memDC.DrawLine(startPoint.x, startPoint.y, endPoint.x, startPoint.y);
                memDC.DrawLine(endPoint.x, startPoint.y, endPoint.x, endPoint.y);
                memDC.DrawLine(endPoint.x, endPoint.y, startPoint.x, endPoint.y);
                memDC.DrawLine(startPoint.x, endPoint.y, startPoint.x, startPoint.y);

                // the pen straddles the drawn coordinate, so the bottom border
                // extends past endPoint.y by roughly half its width
                lastHighlightBottom = std::max(lastHighlightBottom, endPoint.y + highlightPenWidth);
                }
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // always clip dead space at the bottom
    wxString contentBottomOutput;
    long contentBottom{ 0 };
    if (webView->RunScript(L"Math.round(document.documentElement.scrollHeight - "
                           "window.pageYOffset).toString();",
                           &contentBottomOutput) &&
        contentBottomOutput.ToLong(&contentBottom) && contentBottom > 0)
        {
        const int contentBottomDevicePx{ static_cast<int>(contentBottom * dpiScale) };
        if (contentBottomDevicePx < bitmap.GetHeight())
            {
            bitmap = bitmap.GetSubBitmap(wxRect{ 0, 0, bitmap.GetWidth(), contentBottomDevicePx });
            }
        }

    // if clipping to highlights, additionally crop below the last highlighted section
    if (clipContents && !highlightPoints.empty() && lastHighlightBottom != -1 &&
        lastHighlightBottom < bitmap.GetHeight())
        {
        bitmap = bitmap.GetSubBitmap(wxRect{ 0, 0, bitmap.GetWidth(), lastHighlightBottom });
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
wxBitmap Screenshot::CaptureWebViewContent(wxWebView* webView)
    {
    wxClientDC dc(webView);
    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);

    bool contentCaptured{ false };
#if defined(__WXMSW__) && wxUSE_WEBVIEW_EDGE
    // A web view's content is composited outside of what a plain bit-block transfer
    // (what Blit() below falls back to) can see. Asking the browser to render itself
    // to an image directly is the only reliable way to pick that content up.
    wxCOMPtr<IStream> captureStream;
    if (auto* coreWebView2 = static_cast<ICoreWebView2*>(webView->GetNativeBackend());
        coreWebView2 != nullptr &&
        SUCCEEDED(::CreateStreamOnHGlobal(nullptr, TRUE, &captureStream)))
        {
        int captureResult{ -1 };
        const HRESULT hr = coreWebView2->CapturePreview(
            COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG, captureStream,
            Callback<ICoreWebView2CapturePreviewCompletedHandler>(
                [&captureResult](HRESULT errorCode) -> HRESULT
                {
                    captureResult = SUCCEEDED(errorCode) ? 1 : 0;
                    return S_OK;
                })
                .Get());
        if (SUCCEEDED(hr))
            {
            // wait for the capture to complete
            while (captureResult == -1)
                {
                wxYield();
                }
            }
        if (captureResult == 1)
            {
            ULARGE_INTEGER streamSize{};
            LARGE_INTEGER zero{};
            if (SUCCEEDED(captureStream->Seek(zero, STREAM_SEEK_END, &streamSize)) &&
                SUCCEEDED(captureStream->Seek(zero, STREAM_SEEK_SET, nullptr)))
                {
                wxMemoryBuffer pngData(streamSize.QuadPart);
                ULONG bytesRead{ 0 };
                if (SUCCEEDED(captureStream->Read(pngData.GetWriteBuf(streamSize.QuadPart),
                                                  static_cast<ULONG>(streamSize.QuadPart),
                                                  &bytesRead)))
                    {
                    wxImage capturedImage;
                    wxMemoryInputStream pngInput(pngData.GetData(), bytesRead);
                    if (capturedImage.LoadFile(pngInput, wxBITMAP_TYPE_PNG) && capturedImage.IsOk())
                        {
                        bitmap = wxBitmap(capturedImage);
                        contentCaptured = true;
                        }
                    }
                }
            }
        }
#endif

    if (!contentCaptured)
        {
        wxMemoryDC memDC;
        memDC.SelectObject(bitmap);
        memDC.Clear();
        memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);
        memDC.SelectObject(wxNullBitmap);
        }

    return bitmap;
    }

//---------------------------------------------------
void Screenshot::FindVisibleWebViews(wxWindow* parent, std::vector<wxWebView*>& webViews)
    {
    for (auto* child : parent->GetChildren())
        {
        if (child->IsKindOf(CLASSINFO(wxWebView)) && child->IsShownOnScreen())
            {
            webViews.push_back(dynamic_cast<wxWebView*>(child));
            }
        else
            {
            FindVisibleWebViews(child, webViews);
            }
        }
    }

//---------------------------------------------------
void Screenshot::CompositeWebViewsIntoDC(wxWindow* windowToCapture, wxDC& dc)
    {
    if (windowToCapture == nullptr)
        {
        return;
        }

    std::vector<wxWebView*> webViews;
    FindVisibleWebViews(windowToCapture, webViews);

    for (auto* webView : webViews)
        {
        // step back from the web view to the captured window, tallying the offset of
        // each child relative to its parent (using screen positions would be off, since
        // the main dialog's decorations aren't factored into the client area being captured)
        wxPoint offset{ 0, 0 };
        const wxWindow* current = webView;
        while (current != nullptr && current != windowToCapture)
            {
            offset += current->GetPosition();
            current = current->GetParent();
            }

        dc.DrawBitmap(CaptureWebViewContent(webView), offset);
        }
    }

//---------------------------------------------------
wxString Screenshot::GetWebViewTextWalkerScript()
    {
    return LR"JS(
        var walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT,
            { acceptNode: function(node)
                {
                var parent = node.parentElement;
                if (parent && (parent.tagName === 'SCRIPT' || parent.tagName === 'STYLE'))
                    { return NodeFilter.FILTER_REJECT; }
                // skip hover-only tooltip text
                if (parent && parent.closest('.tooltip-box'))
                    { return NodeFilter.FILTER_REJECT; }
                return NodeFilter.FILTER_ACCEPT;
                } });
        )JS";
    }

//---------------------------------------------------
bool Screenshot::FindWebViewTextRange(wxWebView* webView, const wxString& searchText,
                                      const long searchFrom, long& foundStart, long& foundEnd)
    {
    if (webView == nullptr || searchText.empty())
        {
        return false;
        }

    // escape for embedding in a single-quoted JS string literal
    wxString escapedSearchText{ searchText };
    escapedSearchText.Replace(L"\\", L"\\\\", true);
    escapedSearchText.Replace(L"'", L"\\'", true);
    escapedSearchText.Replace(L"\n", L"\\n", true);
    escapedSearchText.Replace(L"\r", L"\\r", true);

    const wxString script = L"(function() {" + GetWebViewTextWalkerScript() +
                            wxString::Format(LR"JS(
                                var text = '', node;
                                while ((node = walker.nextNode())) { text += node.textContent; }
                                var needle = '%s';
                                var idx = text.indexOf(needle, %ld);
                                return (idx === -1) ? '' : (idx + ',' + (idx + needle.length));
                                )JS",
                                             escapedSearchText, searchFrom) +
                            L"})();";

    wxString scriptOutput;
    if (!webView->RunScript(script, &scriptOutput) || scriptOutput.empty())
        {
        return false;
        }

    wxStringTokenizer tkz{ scriptOutput, L",", wxTOKEN_STRTOK };
    long start{ -1 }, end{ -1 };
    if (!tkz.HasMoreTokens() || !tkz.GetNextToken().ToLong(&start) || !tkz.HasMoreTokens() ||
        !tkz.GetNextToken().ToLong(&end))
        {
        return false;
        }

    foundStart = start;
    foundEnd = end;
    return true;
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfDialogWithPropertyGrid(const wxString& filePath,
                                                        const wxWindowID propertyGridId /*= wxID_ANY*/,
                                                        const wxString& startIdToHighlight /*= wxString{}*/,
                                                        wxString endIdToHighlight /*= wxString{}*/,
                                                        const std::pair<bool, wxCoord>& cropToGridHeightAndMinSize /*=
                                                         std::make_pair(false, wxDefaultCoord*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }

    PrepareWindowForScreenshot(windowToCapture);

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    // no alpha channel, just a raw RGB bitmap
    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (propertyGridId != wxID_ANY)
        {
        wxWindow* window = windowToCapture->FindWindow(propertyGridId);
        if (window != nullptr)
            {
            if (endIdToHighlight.empty())
                {
                endIdToHighlight = startIdToHighlight;
                }
            const auto* propertyGridWindow = dynamic_cast<wxPropertyGridInterface*>(window);
            if (propertyGridWindow != nullptr &&
                (propertyGridWindow->GetProperty(wxGetTranslation(startIdToHighlight)) !=
                 nullptr) &&
                (propertyGridWindow->GetProperty(wxGetTranslation(endIdToHighlight)) != nullptr) &&
                (propertyGridWindow->GetState() != nullptr))
                {
                wxPoint startPoint{ 0, 0 };
                auto* startWindowParent = window;
                while (startWindowParent != nullptr && startWindowParent != windowToCapture)
                    {
                    startPoint += startWindowParent->GetPosition();
                    startWindowParent = startWindowParent->GetParent();
                    }
                wxRect rectToHighlight = propertyGridWindow->GetState()->GetGrid()->GetPropertyRect(
                    propertyGridWindow->GetProperty(startIdToHighlight),
                    propertyGridWindow->GetProperty(endIdToHighlight));

                rectToHighlight.Offset(startPoint);
                memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
                memDC.DrawLine(rectToHighlight.GetTopLeft().x, rectToHighlight.GetTopLeft().y,
                               rectToHighlight.GetTopRight().x, rectToHighlight.GetTopRight().y);
                memDC.DrawLine(rectToHighlight.GetTopRight().x, rectToHighlight.GetTopRight().y,
                               rectToHighlight.GetBottomRight().x,
                               rectToHighlight.GetBottomRight().y);
                memDC.DrawLine(
                    rectToHighlight.GetBottomRight().x, rectToHighlight.GetBottomRight().y,
                    rectToHighlight.GetBottomLeft().x, rectToHighlight.GetBottomLeft().y);
                memDC.DrawLine(rectToHighlight.GetBottomLeft().x, rectToHighlight.GetBottomLeft().y,
                               rectToHighlight.GetTopLeft().x, rectToHighlight.GetTopLeft().y);
                }
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // crop vertically, if requested
    if (cropToGridHeightAndMinSize.first && propertyGridId != wxID_ANY)
        {
        wxWindow* window = windowToCapture->FindWindow(propertyGridId);
        if (window != nullptr)
            {
            const auto* propertyGridWindow = dynamic_cast<wxPropertyGridInterface*>(window);
            if (propertyGridWindow != nullptr)
                {
                const wxRect gridRect = propertyGridWindow->GetState()->GetGrid()->GetPropertyRect(
                    propertyGridWindow->GetState()->GetGrid()->GetRoot(),
                    propertyGridWindow->GetState()->GetGrid()->GetLastItem());
                bitmap = bitmap.GetSubBitmap(wxRect{
                    0, 0, bitmap.GetWidth(),
                    std::max(GetActiveDialogOrFrame()->FromDIP(cropToGridHeightAndMinSize.second),
                             gridRect.GetHeight()) });
                }
            }
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshot(const wxString& filePath,
                                const wxWindowID startIdToHighlight /*= wxID_ANY*/,
                                const wxWindowID endIdToHighlight /*= wxID_ANY*/,
                                const wxWindowID cutoffId /*= wxID_ANY*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }

    PrepareWindowForScreenshot(windowToCapture);

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    // use 24-bit (RGB) bitmap, because including the alpha channel
    // is unnecessary and causes artifacts on HiDPI displays
    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);
    // a plain blit leaves any web view's area blank, since its content is composited
    // outside of what that can see
    CompositeWebViewsIntoDC(windowToCapture, memDC);

    wxCoord endPointY{ 0 };

    wxPoint startPoint{ 0, 0 };

    if (startIdToHighlight != wxID_ANY || endIdToHighlight != wxID_ANY)
        {
        const wxWindow* startWindow = (startIdToHighlight == wxID_ANY) ?
                                          nullptr :
                                          windowToCapture->FindWindow(startIdToHighlight);
        if (startWindow != nullptr)
            {
            /* Step back all the way from the child window to the parent and tally the offset
               of the children relative to its parent. When dealing with client areas, using
               the screen position of controls will be off because the main dialog's decorations
               aren't factored into that.*/
            const auto* startWindowParent = startWindow;
            while (startWindowParent != nullptr && startWindowParent != windowToCapture)
                {
                startPoint += startWindowParent->GetPosition();
                startWindowParent = startWindowParent->GetParent();
                }
            wxPoint endPoint{ startPoint.x + startWindow->GetSize().GetWidth(),
                              startPoint.y + startWindow->GetSize().GetHeight() };
            const wxWindow* endWindow = (endIdToHighlight == wxID_ANY) ?
                                            nullptr :
                                            windowToCapture->FindWindow(endIdToHighlight);
            if (endWindow != nullptr)
                {
                endPoint = wxPoint{ 0, 0 };
                const auto* endWindowParent = endWindow;
                while (endWindowParent != nullptr && endWindowParent != windowToCapture)
                    {
                    endPoint += endWindowParent->GetPosition();
                    endWindowParent = endWindowParent->GetParent();
                    }
                // bump down the highlighting to include the end control also
                endPoint += endWindow->GetSize();
                }
            // add a little padding around the control(s) being highlighted
            startPoint -=
                wxPoint{ wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder() };
            endPoint += wxPoint{
                // same for end point, but make sure we didn't go off the screen
                (endPoint.x + wxSizerFlags::GetDefaultBorder() < memDC.GetSize().GetWidth()) ?
                    wxSizerFlags::GetDefaultBorder() :
                    0,
                (endPoint.y + wxSizerFlags::GetDefaultBorder() < memDC.GetSize().GetHeight()) ?
                    wxSizerFlags::GetDefaultBorder() :
                    0
            };
            endPointY = endPoint.y;
            memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
            memDC.DrawLine(startPoint.x, startPoint.y, endPoint.x, startPoint.y);
            memDC.DrawLine(endPoint.x, startPoint.y, endPoint.x, endPoint.y);
            memDC.DrawLine(endPoint.x, endPoint.y, startPoint.x, endPoint.y);
            memDC.DrawLine(startPoint.x, endPoint.y, startPoint.x, startPoint.y);
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // crop vertically, if requested
    if (cutoffId != wxID_ANY)
        {
        const wxWindow* cutoffWindow = windowToCapture->FindWindow(cutoffId);
        if (cutoffWindow != nullptr)
            {
            wxPoint cutoffPoint{ 0, 0 };
            const auto* cutoffWindowParent = cutoffWindow;
            while ((cutoffWindowParent != nullptr) && cutoffWindowParent != windowToCapture)
                {
                cutoffPoint += cutoffWindowParent->GetPosition();
                cutoffWindowParent = cutoffWindowParent->GetParent();
                }
            const wxPoint cutOffEndPoint{ cutoffPoint.x + cutoffWindow->GetSize().GetWidth(),
                                          cutoffPoint.y + cutoffWindow->GetSize().GetHeight() };

            // if cutoff is at the starting point (or above it),
            // then crop above the first highlighted control
            if (startPoint.y >= cutOffEndPoint.y)
                {
                const wxCoord yStart = cutOffEndPoint.y - wxSizerFlags::GetDefaultBorder();
                bitmap = bitmap.GetSubBitmap(
                    wxRect{ 0, yStart, bitmap.GetWidth(), bitmap.GetHeight() - yStart });
                }
            // ...otherwise, crop beneath end point
            else
                {
                bitmap = bitmap.GetSubBitmap(wxRect{ 0, 0, bitmap.GetWidth(),
                                                     // if there is something being highlighted,
                                                     // make sure we don't cut that off
                                                     std::max(cutOffEndPoint.y, endPointY) +
                                                         wxSizerFlags::GetDefaultBorder() });
                }
            }
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshot(const wxString& filePath, const wxString& annotation,
                                const wxWindowID startIdToOverwrite,
                                const wxWindowID endIdToOverwrite /*= wxID_ANY*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        {
        windowToCapture = wxTopLevelWindows.GetLast()->GetData();
        }
    if (windowToCapture == nullptr)
        {
        return false;
        }

    PrepareWindowForScreenshot(windowToCapture);

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    // use 24-bit (RGB) bitmap, because including the alpha channel
    // is unnecessary and causes artifacts on HiDPI displays
    wxBitmap bitmap(dc.GetSize(), RGB_CHANNEL_SIZE);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);
    // a plain blit leaves any web view's area blank, since its content is composited
    // outside of what that can see
    CompositeWebViewsIntoDC(windowToCapture, memDC);

    if (startIdToOverwrite != wxID_ANY || endIdToOverwrite != wxID_ANY)
        {
        const wxWindow* startWindow = (startIdToOverwrite == wxID_ANY) ?
                                          nullptr :
                                          windowToCapture->FindWindow(startIdToOverwrite);
        if (startWindow != nullptr)
            {
            /* Step back all the way from the child window to the parent and tally the offset
               of the children relative to its parent. When dealing with client areas, using
               the screen position of controls will be off because the main dialog's decorations
               aren't factored into that.*/
            wxPoint startPoint{ 0, 0 };
            const auto* startWindowParent = startWindow;
            while (startWindowParent != nullptr && startWindowParent != windowToCapture)
                {
                startPoint += startWindowParent->GetPosition();
                startWindowParent = startWindowParent->GetParent();
                }
            wxPoint endPoint{ startPoint.x + startWindow->GetSize().GetWidth(),
                              startPoint.y + startWindow->GetSize().GetHeight() };
            const wxWindow* endWindow = (endIdToOverwrite == wxID_ANY) ?
                                            nullptr :
                                            windowToCapture->FindWindow(endIdToOverwrite);
            if (endWindow != nullptr)
                {
                endPoint = wxPoint{ 0, 0 };
                const auto* endWindowParent = endWindow;
                while (endWindowParent != nullptr && endWindowParent != windowToCapture)
                    {
                    endPoint += endWindowParent->GetPosition();
                    endWindowParent = endWindowParent->GetParent();
                    }
                // bump down the highlighting to include the end control also
                endPoint += endWindow->GetSize();
                }

            memDC.SetPen(GetOutlinePen(windowToCapture->GetDPIScaleFactor()));
            memDC.SetBrush(wxColour{ 255, 255, 255 });
            memDC.DrawRectangle(
                wxRect{ wxPoint{ startPoint.x, startPoint.y }, wxPoint{ endPoint.x, endPoint.y } });
            memDC.DrawText(annotation, wxPoint(startPoint.x + 2, startPoint.y + 2));
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
void Screenshot::PrepareWindowForScreenshot(wxWindow* windowToCapture)
    {
    windowToCapture->Refresh(true);
    windowToCapture->UpdateWindowUI(wxUPDATE_UI_RECURSE);
    wxTheApp->Yield();
    ::wxSleep(2);
    }

//---------------------------------------------------
wxWindow* Screenshot::GetActiveDialogOrFrame()
    {
    // NOLINTNEXTLINE(misc-const-correctness)
    wxWindow* focusWindow = wxWindow::FindFocus();
    if (focusWindow != nullptr)
        {
        while (focusWindow != nullptr && !focusWindow->IsKindOf(CLASSINFO(wxDialog)) &&
               !focusWindow->IsKindOf(CLASSINFO(wxFrame)))
            {
            focusWindow = focusWindow->GetParent();
            }
        }
    return (focusWindow != nullptr) ? focusWindow : wxGetActiveWindow();
    }

// NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
