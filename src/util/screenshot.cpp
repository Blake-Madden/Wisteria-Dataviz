///////////////////////////////////////////////////////////////////////////////
// Name:        screenshot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "screenshot.h"

//---------------------------------------------------
bool Screenshot::ConvertImageToPng(const wxString& filePath, const wxSize scaledSize,
    const bool removeOriginalFile /*= false*/)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        wxFileName fn(filePath);
        fn.SetExt(L"png");
        wxImage img(bmp.ConvertToImage());
        const auto [newWidth, newHeight] = geometry::downscaled_size(
            std::make_pair(img.GetWidth(), img.GetHeight()),
            std::make_pair(scaledSize.GetWidth(), scaledSize.GetHeight()));
        if (!img.Rescale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH).
            SaveFile(fn.GetFullPath(), wxBitmapType::wxBITMAP_TYPE_PNG))
            {
            wxLogWarning(L"Unable to save '%s' when converting screenshot.", fn.GetFullPath());
            return false;
            }
        if (removeOriginalFile)
            {
            if (!wxRemoveFile(filePath))
                { wxLogWarning(L"Unable to delete '%s' when converting screenshot.", filePath); }
            }
        return true;
        }
    else
        { return false; }
    }

//---------------------------------------------------
bool Screenshot::HighlightItemInScreenshot(const wxString& filePath,
                                           const wxPoint topLeftCorner,
                                           const wxPoint bottomRightCorner)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        wxMemoryDC memDC;
        memDC.SelectObject(bmp);
        memDC.SetPen(GetScreenshotHighlightPen(wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
        memDC.DrawLine(topLeftCorner.x, topLeftCorner.y,
                       bottomRightCorner.x, topLeftCorner.y);
        memDC.DrawLine(bottomRightCorner.x, topLeftCorner.y,
                       bottomRightCorner.x, bottomRightCorner.y);
        memDC.DrawLine(bottomRightCorner.x, bottomRightCorner.y,
                       topLeftCorner.x, bottomRightCorner.y);
        memDC.DrawLine(topLeftCorner.x, bottomRightCorner.y,
                       topLeftCorner.x, topLeftCorner.y);
        memDC.SelectObject(wxNullBitmap);

        return bmp.SaveFile(filePath, wxBitmapType::wxBITMAP_TYPE_BMP);
        }
    else
        { return false; }
    }

//---------------------------------------------------
bool Screenshot::AnnotateScreenshot(const wxString& filePath,
                                    const wxString& text,
                                    const wxPoint topLeftCorner,
                                    const wxPoint bottomRightCorner)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        wxMemoryDC memDC;
        memDC.SelectObject(bmp);
        memDC.SetPen(GetOutlintPen(wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
        memDC.SetBrush(*wxWHITE);
        memDC.DrawRectangle(wxRect(topLeftCorner, bottomRightCorner));
        memDC.DrawText(text, topLeftCorner);

        memDC.SelectObject(wxNullBitmap);

        return bmp.SaveFile(filePath, wxBitmapType::wxBITMAP_TYPE_BMP);
        }
    else
        { return false; }
    }

//---------------------------------------------------
bool Screenshot::CropScreenshot(const wxString& filePath,
                                wxCoord width, wxCoord height)
    {
    wxBitmap bmp(filePath, wxBITMAP_TYPE_ANY);
    if (bmp.IsOk())
        {
        if (width == wxDefaultCoord)
            { width = bmp.GetWidth(); }
        if (height == wxDefaultCoord)
            { height = bmp.GetHeight(); }
        bmp = bmp.GetSubBitmap(wxRect{ 0, 0, width, height });

        AddBorderToImage(bmp);

        return bmp.SaveFile(filePath, wxBitmapType::wxBITMAP_TYPE_BMP);
        }
    else
        { return false; }
    }

//---------------------------------------------------
void Screenshot::AddBorderToImage(wxBitmap& bmp) // cppcheck-suppress constParameter
    {
    wxMemoryDC memDC;
    memDC.SelectObject(bmp);

    memDC.SetPen(wxPen(*wxLIGHT_GREY, wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
    const wxPoint corners[] = { wxPoint(0,0),
                                wxPoint(memDC.GetSize().GetWidth()-memDC.GetPen().GetWidth(), 0),
                                wxPoint(memDC.GetSize().GetWidth()-memDC.GetPen().GetWidth(),
                                        memDC.GetSize().GetHeight()-memDC.GetPen().GetWidth()),
                                wxPoint(0, memDC.GetSize().GetHeight()-memDC.GetPen().GetWidth()),
                                wxPoint(0,0) };
    memDC.DrawLines(std::size(corners), corners);
    memDC.SelectObject(wxNullBitmap);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfRibbon(const wxString& filePath,
                                        const int pageToSelect /*= 0*/,
                                        const wxWindowID buttonBarToHighlight /*= wxID_ANY*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        { windowToCapture = wxTopLevelWindows.GetLast()->GetData(); }
    if (windowToCapture == nullptr)
        { return false; }

    wxWindow* foundWindow{ nullptr };
    if (!windowToCapture->IsKindOf(CLASSINFO(wxRibbonBar)))
        {
        auto& children = windowToCapture->GetChildren();
        for (const auto& child : children)
            {
            if (child && child->IsKindOf(CLASSINFO(wxRibbonBar)))
                {
                foundWindow = child;
                break;
                }
            }
        }
    if (!foundWindow)
        { return false; }

    auto ribbonBar = dynamic_cast<wxRibbonBar*>(foundWindow);
    assert(ribbonBar);

    ribbonBar->SetActivePage(pageToSelect);
    wxTheApp->Yield();

    wxClientDC dc(ribbonBar);
    wxMemoryDC memDC;

    wxBitmap bitmap(dc.GetSize(), 24);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (buttonBarToHighlight != wxID_ANY)
        {
        const auto buttonBar = ribbonBar->FindWindow(buttonBarToHighlight);
        if (buttonBar && buttonBar->IsKindOf(CLASSINFO(wxRibbonButtonBar)))
            {
            /* Step back all the way from the child window to the parent and tally the offset
               of the children relative to its parent. When dealing with client areas, using
               the screen position of controls will be off because the main dialog's decorations
               aren't factored into that.*/
            wxPoint startPoint(0, 0);
            auto startWindowParent = buttonBar;
            while (startWindowParent && startWindowParent != ribbonBar)
                {
                startPoint += startWindowParent->GetPosition();
                startWindowParent = startWindowParent->GetParent();
                }
            wxPoint endPoint(startPoint.x + buttonBar->GetSize().GetWidth(),
                             startPoint.y + buttonBar->GetSize().GetHeight());
            // add a little padding around the control(s) being highlighted
            startPoint -= wxPoint(wxSizerFlags::GetDefaultBorder(),
                                  wxSizerFlags::GetDefaultBorder());
            endPoint += wxPoint(
                // same for end point, but make sure we didn't go off the screen
                (endPoint.x + wxSizerFlags::GetDefaultBorder() < memDC.GetSize().GetWidth()) ?
                    wxSizerFlags::GetDefaultBorder() : 0,
                (endPoint.y + wxSizerFlags::GetDefaultBorder() < memDC.GetSize().GetHeight()) ?
                    wxSizerFlags::GetDefaultBorder() : 0);
            memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
            memDC.DrawLine(startPoint.x, startPoint.y, endPoint.x, startPoint.y);
            memDC.DrawLine(endPoint.x, startPoint.y, endPoint.x, endPoint.y);
            memDC.DrawLine(endPoint.x, endPoint.y, startPoint.x, endPoint.y);
            memDC.DrawLine(startPoint.x, endPoint.y, startPoint.x, startPoint.y);
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    // create the folder to the filepath--if necessary--and save the image
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfListControl(const wxString& filePath,
                                             const wxWindowID windowId,
                                             const long startRow /*= -1*/,
                                             const long endRow /*= -1*/,
                                             const long startColumn /*= -1*/,
                                             const long endColumn /*= -1*/,
                                             const long cuttOffRow /*= -1*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        { windowToCapture = wxTopLevelWindows.GetLast()->GetData(); }
    if (windowToCapture == nullptr)
        { return false; }
    if (windowToCapture->GetId() != windowId ||
        !windowToCapture->IsKindOf(CLASSINFO(wxListCtrl)))
        {
        wxWindow* foundWindow = windowToCapture->FindWindow(windowId);
        if (foundWindow != nullptr && foundWindow->IsKindOf(CLASSINFO(wxListCtrl)))
            { windowToCapture = foundWindow; }
        else
            { return false; }
        }
    auto listCtrl = dynamic_cast<wxListCtrl*>(windowToCapture);
    assert(listCtrl);

    long columnsWidth{0};
    for (auto i = 0; i < listCtrl->GetColumnCount(); ++i)
        { columnsWidth += listCtrl->GetColumnWidth(i); }
    long rowHeight{0};
    if (listCtrl->GetItemCount())
        {
        wxRect itemRect;
        listCtrl->GetItemRect(0, itemRect);
        rowHeight = itemRect.GetHeight()*(listCtrl->GetItemCount() + 1.5/*header*/);
        }

    if (endRow != -1)
        {
        listCtrl->EnsureVisible(endRow);
        listCtrl->EnsureVisible(startRow);
        wxTheApp->Yield();
        }
    if (cuttOffRow != -1)
        {
        listCtrl->EnsureVisible(cuttOffRow);
        wxTheApp->Yield();
        }

    wxClientDC dc(listCtrl);
    wxMemoryDC memDC;

    wxBitmap bitmap(dc.GetSize(), 24);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (startRow != -1 || endRow != -1 ||
        startColumn != -1 || endColumn != -1)
        {
        wxRect startRect, endRect;
        if (listCtrl &&
            listCtrl->GetSubItemRect((startRow == -1 ? 0 : startRow),
                                     (startColumn == -1 ? 0 : startColumn), startRect) &&
            listCtrl->GetSubItemRect((endRow == -1 ? listCtrl->GetItemCount()-1 : endRow),
                                     (endColumn == -1 ? listCtrl->GetColumnCount()-1 : endColumn),
                                      endRect))
            {
            wxRect highlightRect(startRect.GetTopLeft(), endRect.GetBottomRight());
            highlightRect.x += listCtrl->GetScrollPos(wxHORIZONTAL);
            highlightRect.y += listCtrl->GetScrollPos(wxVERTICAL);
            wxDCPenChanger pc(memDC,
                GetScreenshotHighlightPen(wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
            wxDCBrushChanger bc(memDC, *wxTRANSPARENT_BRUSH);
            memDC.DrawRectangle(highlightRect);
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // chop off rows that user doesn't want included
    if (cuttOffRow != -1 && cuttOffRow+1 < listCtrl->GetItemCount())
        {
        wxRect cuttOffRect;
        if (listCtrl &&
            listCtrl->GetSubItemRect(
                // get the top of the row below the cut off
                cuttOffRow + 1,
                0, cuttOffRect))
            {
            bitmap = bitmap.GetSubBitmap(
                wxRect(0, 0, bitmap.GetWidth(), cuttOffRect.GetTop()));
            }
        }
    // chop off any dead space after last column
    if (columnsWidth < bitmap.GetWidth())
        {
        bitmap = bitmap.GetSubBitmap(
                wxRect(0, 0,
                    columnsWidth +
                    // space for the pen if we are right on the edge
                    wxTheApp->GetTopWindow()->GetDPIScaleFactor(),
                    bitmap.GetHeight()));
        }
    // and below last row
    // (this assumes there are less rows in the entire list that fix on the screen)
    if (rowHeight < bitmap.GetHeight())
        {
        bitmap = bitmap.GetSubBitmap(
                wxRect(0, 0, bitmap.GetWidth(),
                       rowHeight + wxTheApp->GetTopWindow()->GetDPIScaleFactor()));
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    // create the folder to the filepath--if necessary--and save the image
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfTextWindow(const wxString& filePath,
                                            const wxWindowID windowId,
                                            const bool clipContents,
                                            const std::vector<std::pair<long,long>>& highlightPoints)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        { windowToCapture = wxTopLevelWindows.GetLast()->GetData(); }
    if (windowToCapture == nullptr)
        { return false; }
    if (windowToCapture->GetId() != windowId ||
        !windowToCapture->IsKindOf(CLASSINFO(wxTextCtrl)))
        {
        wxWindow* foundWindow = windowToCapture->FindWindow(windowId);
        if (foundWindow && foundWindow->IsKindOf(CLASSINFO(wxTextCtrl)))
            { windowToCapture = foundWindow; }
        else
            { return false; }
        }

    PrepareWindowForScreenshot(windowToCapture);

    if (highlightPoints.size())
        { dynamic_cast<wxTextCtrl*>(windowToCapture)->ShowPosition(highlightPoints[0].first); }

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    wxBitmap bitmap(dc.GetSize(), 24);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    const wxTextCtrl* textWindow = dynamic_cast<wxTextCtrl*>(windowToCapture);

    for (size_t i = 0; i < highlightPoints.size(); ++i)
        {
        if (textWindow)
            {
            wxTextAttr style;
            wxPoint startPoint = textWindow->PositionToCoords(highlightPoints[i].first);
            wxPoint endPoint = (highlightPoints[i].second != -1) ?
                textWindow->PositionToCoords(highlightPoints[i].second) :
                textWindow->PositionToCoords(textWindow->GetLastPosition());
            // if points are on different lines, then highlight the whole row
            if (startPoint.y != endPoint.y)
                {
                startPoint.x = 0;
                endPoint.x = memDC.GetSize().GetWidth()-memDC.GetPen().GetWidth();
                }
            long x{ 0 }, y{ 0 };
            if ((highlightPoints[i].second != -1) &&
                textWindow->PositionToXY(highlightPoints[i].second, &x, &y))
                { endPoint.y = textWindow->PositionToCoords(textWindow->XYToPosition(0,y+1)).y; }
            else
                { endPoint.y += (textWindow->GetDefaultStyle().GetFontSize()*2); }
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

    // chop off whitespace if we scrolled to bottom of the file
    if (clipContents)
        {
        wxPoint endOfWindowPoint =
            textWindow->PositionToCoords(textWindow->GetLastPosition());
        endOfWindowPoint.y += (textWindow->GetDefaultStyle().GetFontSize() * 2);
        if (endOfWindowPoint.y < bitmap.GetHeight())
            {
            bitmap = bitmap.GetSubBitmap(
                wxRect(0, 0, bitmap.GetWidth(), endOfWindowPoint.y));
            }
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    // create the folder to the filepath--if necessary--and save the image
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshotOfPropertyGrid(const wxString& filePath,
                                              const wxWindowID propertyGridId /*= wxID_ANY*/,
                                              const wxString& startIdToHighlight /*= wxEmptyString*/,
                                              wxString endIdToHighlight /*= wxEmptyString*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        { windowToCapture = wxTopLevelWindows.GetLast()->GetData(); }
    if (windowToCapture == nullptr)
        { return false; }

    PrepareWindowForScreenshot(windowToCapture);

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    // no alpha channel, just a raw RGB bitmap
    wxBitmap bitmap(dc.GetSize(), 24);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (propertyGridId != wxID_ANY)
        {
        wxWindow* window = windowToCapture->FindWindow(propertyGridId);
        if (window != nullptr && window->IsKindOf(CLASSINFO(wxPropertyGridManager)))
            {
            if (endIdToHighlight.empty())
                { endIdToHighlight = startIdToHighlight; }
            const wxPropertyGridManager* propertyGridWindow =
                dynamic_cast<wxPropertyGridManager*>(window);
            if (propertyGridWindow->GetProperty(wxGetTranslation(startIdToHighlight)) &&
                propertyGridWindow->GetProperty(wxGetTranslation(endIdToHighlight)) &&
                propertyGridWindow->GetState())
                {
                wxPoint startPoint(0, 0);
                auto startWindowParent = window;
                while (startWindowParent && startWindowParent != windowToCapture)
                    {
                    startPoint += startWindowParent->GetPosition();
                    startWindowParent = startWindowParent->GetParent();
                    }
                wxRect rectToHighlight =
                    propertyGridWindow->GetState()->GetGrid()->
                        GetPropertyRect(propertyGridWindow->GetProperty(startIdToHighlight),
                            propertyGridWindow->GetProperty(endIdToHighlight));
                rectToHighlight.Offset(startPoint);
                memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
                memDC.DrawLine(rectToHighlight.GetTopLeft().x,
                               rectToHighlight.GetTopLeft().y,
                               rectToHighlight.GetTopRight().x,
                               rectToHighlight.GetTopRight().y);
                memDC.DrawLine(rectToHighlight.GetTopRight().x,
                    rectToHighlight.GetTopRight().y,
                    rectToHighlight.GetBottomRight().x,
                    rectToHighlight.GetBottomRight().y);
                memDC.DrawLine(rectToHighlight.GetBottomRight().x,
                               rectToHighlight.GetBottomRight().y,
                               rectToHighlight.GetBottomLeft().x,
                               rectToHighlight.GetBottomLeft().y);
                memDC.DrawLine(rectToHighlight.GetBottomLeft().x,
                               rectToHighlight.GetBottomLeft().y,
                               rectToHighlight.GetTopLeft().x,
                               rectToHighlight.GetTopLeft().y);
                }
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    // create the folder to the filepath--if necessary--and save the image
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
        { windowToCapture = wxTopLevelWindows.GetLast()->GetData(); }
    if (windowToCapture == nullptr)
        { return false; }

    PrepareWindowForScreenshot(windowToCapture);

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    // use 24-bit (RGB) bitmap, because including the alpha channel
    // is unnecessary and causes artifacts on HiDPI displays.
    wxBitmap bitmap(dc.GetSize(), 24);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    /* On Vista and above, the Aero theme breaks ::BitBlt grabbing the entire window
       (the outer frame appears translucent).
       Instead, BitBlit the client area and not the entire dialog.
       An alternative is to call this:

       ::PrintWindow(windowToCapture->GetHandle(), memDC.GetHDC(), 0)

       But on HiDPI displays that causes various artifacts in the images.
       BitBlitting the client area is less problematic overall.*/
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (startIdToHighlight != wxID_ANY || endIdToHighlight != wxID_ANY)
        {
        const wxWindow* startWindow = (startIdToHighlight == wxID_ANY) ?
            nullptr :
            windowToCapture->FindWindow(startIdToHighlight);
        if (startWindow)
            {
            /* Step back all the way from the child window to the parent and tally the offset
               of the children relative to its parent. When dealing with client areas, using
               the screen position of controls will be off because the main dialog's decorations
               aren't factored into that.*/
            wxPoint startPoint(0, 0);
            auto startWindowParent = startWindow;
            while (startWindowParent && startWindowParent != windowToCapture)
                {
                startPoint += startWindowParent->GetPosition();
                startWindowParent = startWindowParent->GetParent();
                }
            wxPoint endPoint(startPoint.x + startWindow->GetSize().GetWidth(),
                             startPoint.y + startWindow->GetSize().GetHeight());
            const wxWindow* endWindow = (endIdToHighlight == wxID_ANY) ?
                nullptr :
                windowToCapture->FindWindow(endIdToHighlight);
            if (endWindow)
                {
                endPoint = wxPoint(0, 0);
                auto endWindowParent = endWindow;
                while (endWindowParent && endWindowParent != windowToCapture)
                    {
                    endPoint += endWindowParent->GetPosition();
                    endWindowParent = endWindowParent->GetParent();
                    }
                // bump down the highlighting to include the end control also
                endPoint += endWindow->GetSize();
                }
            // add a little padding around the control(s) being highlighted
            startPoint -= wxPoint(wxSizerFlags::GetDefaultBorder(),
                                  wxSizerFlags::GetDefaultBorder());
            endPoint += wxPoint(
                // same for end point, but make sure we didn't go off the screen
                (endPoint.x + wxSizerFlags::GetDefaultBorder() < memDC.GetSize().GetWidth()) ?
                wxSizerFlags::GetDefaultBorder() : 0,
                (endPoint.y + wxSizerFlags::GetDefaultBorder() < memDC.GetSize().GetHeight()) ?
                wxSizerFlags::GetDefaultBorder() : 0);
            memDC.SetPen(GetScreenshotHighlightPen(windowToCapture->GetDPIScaleFactor()));
            memDC.DrawLine(startPoint.x, startPoint.y, endPoint.x, startPoint.y);
            memDC.DrawLine(endPoint.x, startPoint.y, endPoint.x, endPoint.y);
            memDC.DrawLine(endPoint.x, endPoint.y, startPoint.x, endPoint.y);
            memDC.DrawLine(startPoint.x, endPoint.y, startPoint.x, startPoint.y);
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // crop vertially, if requested
    if (cutoffId != wxID_ANY)
        {
        const wxWindow* cutoffWindow = windowToCapture->FindWindow(cutoffId);
        if (cutoffWindow)
            {
            wxPoint cutoffPoint(0, 0);
            auto cutoffWindowParent = cutoffWindow;
            while (cutoffWindowParent && cutoffWindowParent != windowToCapture)
                {
                cutoffPoint += cutoffWindowParent->GetPosition();
                cutoffWindowParent = cutoffWindowParent->GetParent();
                }
            wxPoint cutOffEndPoint(cutoffPoint.x + cutoffWindow->GetSize().GetWidth(),
                                   cutoffPoint.y + cutoffWindow->GetSize().GetHeight());
            bitmap = bitmap.GetSubBitmap(
                wxRect(0, 0, bitmap.GetWidth(), cutOffEndPoint.y + wxSizerFlags::GetDefaultBorder()));
            }
        }

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    // create the folder to the filepath--if necessary--and save the image
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
bool Screenshot::SaveScreenshot(const wxString& filePath,
                                const wxString& annotation,
                                const wxWindowID startIdToOverwrite,
                                const wxWindowID endIdToOverwrite /*= wxID_ANY*/)
    {
    wxWindow* windowToCapture = GetActiveDialogOrFrame();
    if (windowToCapture == nullptr && wxTopLevelWindows.GetCount() > 0)
        { windowToCapture = wxTopLevelWindows.GetLast()->GetData(); }
    if (windowToCapture == nullptr)
        { return false; }

    PrepareWindowForScreenshot(windowToCapture);

    wxClientDC dc(windowToCapture);
    wxMemoryDC memDC;

    // use 24-bit (RGB) bitmap, because including the alpha channel
    // is unnecessary and causes artifacts on HiDPI displays.
    wxBitmap bitmap(dc.GetSize(), 24);
    memDC.SelectObject(bitmap);
    memDC.Clear();
    /* On Vista and above, the Aero theme breaks ::BitBlt grabbing the entire window
       (the outer frame appears translucent).
       Instead, BitBlit the client area and not the entire dialog.
       An alternative is to call this:

       ::PrintWindow(windowToCapture->GetHandle(), memDC.GetHDC(), 0)

       But on HiDPI displays that causes various artifacts in the images.
       BitBlitting the client area is less problematic overall.*/
    memDC.Blit(0, 0, dc.GetSize().GetWidth(), dc.GetSize().GetHeight(), &dc, 0, 0);

    if (startIdToOverwrite != wxID_ANY || endIdToOverwrite != wxID_ANY)
        {
        const wxWindow* startWindow = (startIdToOverwrite == wxID_ANY) ?
            nullptr :
            windowToCapture->FindWindow(startIdToOverwrite);
        if (startWindow)
            {
            /* Step back all the way from the child window to the parent and tally the offset
               of the children relative to its parent. When dealing with client areas, using
               the screen position of controls will be off because the main dialog's decorations
               aren't factored into that.*/
            wxPoint startPoint(0, 0);
            auto startWindowParent = startWindow;
            while (startWindowParent && startWindowParent != windowToCapture)
                {
                startPoint += startWindowParent->GetPosition();
                startWindowParent = startWindowParent->GetParent();
                }
            wxPoint endPoint(startPoint.x + startWindow->GetSize().GetWidth(),
                             startPoint.y + startWindow->GetSize().GetHeight());
            const wxWindow* endWindow = (endIdToOverwrite == wxID_ANY) ?
                nullptr :
                windowToCapture->FindWindow(endIdToOverwrite);
            if (endWindow)
                {
                endPoint = wxPoint(0, 0);
                auto endWindowParent = endWindow;
                while (endWindowParent && endWindowParent != windowToCapture)
                    {
                    endPoint += endWindowParent->GetPosition();
                    endWindowParent = endWindowParent->GetParent();
                    }
                // bump down the highlighting to include the end control also
                endPoint += endWindow->GetSize();
                }

            memDC.SetPen(GetOutlintPen(windowToCapture->GetDPIScaleFactor()));
            memDC.SetBrush(*wxWHITE);
            memDC.DrawRectangle(wxRect(wxPoint(startPoint.x, startPoint.y), wxPoint(endPoint.x, endPoint.y)));
            memDC.DrawText(annotation, wxPoint(startPoint.x + 2, startPoint.y + 2));
            }
        }

    memDC.SelectObject(wxNullBitmap);

    // draw a gray border around the image since we are saving the client area
    AddBorderToImage(bitmap);

    wxFileName fn(filePath);
    fn.SetExt(L"bmp");
    // create the folder to the filepath--if necessary--and save the image
    wxFileName::Mkdir(fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return bitmap.SaveFile(fn.GetFullPath(), wxBITMAP_TYPE_BMP);
    }

//---------------------------------------------------
void Screenshot::PrepareWindowForScreenshot(wxWindow* windowToCapture)
    {
    windowToCapture->Refresh(true);
    windowToCapture->UpdateWindowUI(wxUPDATE_UI_RECURSE);
    wxTheApp->Yield();
    ::wxSleep(1);
    }

//---------------------------------------------------
wxWindow* Screenshot::GetActiveDialogOrFrame()
    {
    wxWindow* focusWindow = wxWindow::FindFocus();
    if (focusWindow != nullptr)
        {
        while (focusWindow != nullptr &&
                !focusWindow->IsKindOf(CLASSINFO(wxDialog)) &&
                !focusWindow->IsKindOf(CLASSINFO(wxFrame)))
            { focusWindow = focusWindow->GetParent(); }
        }
    return (focusWindow != nullptr) ? focusWindow : wxGetActiveWindow();
    }
