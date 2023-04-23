///////////////////////////////////////////////////////////////////////////////
// Name:        thumbnail.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "thumbnail.h"

namespace Wisteria::UI
    {
    //----------------------------------
    EnlargedImageWindow::EnlargedImageWindow(const wxBitmap& bitmap,
        wxWindow* parent, wxWindowID id /*= wxID_ANY*/,
        const wxPoint& pos /*= wxDefaultPosition*/,
        const wxSize& size /*= wxDefaultSize*/,
        long style /*= wxFRAME_NO_TASKBAR|wxSTAY_ON_TOP|wxFULL_REPAINT_ON_RESIZE*/) :
        wxDialog(parent, id, wxString{}, pos, size, style, L"EnlargedImageWindow"),
        m_bitmap(bitmap)
        {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetSize(GetBitmap().GetSize());
        Centre();

        Bind(wxEVT_CHAR_HOOK, &EnlargedImageWindow::OnChar, this);
        Bind(wxEVT_CHAR, &EnlargedImageWindow::OnChar, this);
        Bind(wxEVT_LEFT_DOWN, &EnlargedImageWindow::OnClick, this);
        Bind(wxEVT_PAINT, &EnlargedImageWindow::OnPaint, this);
        }

    //----------------------------------
    void EnlargedImageWindow::OnClick(wxMouseEvent& event)
        {
        if (event.LeftDown() || event.RightDown())
            {
            if (IsModal())
                { EndModal(wxID_OK); }
            else
                { Close(); }
            }
        }

    //----------------------------------
    void EnlargedImageWindow::OnChar([[maybe_unused]] wxKeyEvent& event)
        {
        if (IsModal())
            { EndModal(wxID_OK); }
        else
            { Close(); }
        }

    //----------------------------------
    void EnlargedImageWindow::OnPaint([[maybe_unused]] wxPaintEvent& event)
        {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        dc.DrawBitmap(GetBitmap(), 0, 0);
        }

    //----------------------------------
    bool DropThumbnailImageFile::OnDropFiles([[maybe_unused]] wxCoord x,
        [[maybe_unused]] wxCoord y, const wxArrayString &filenames)
        {
        if (filenames.size() && wxFileName::FileExists(filenames[0]))
            {
            wxImage img(filenames[0]);
            if (m_pOwner && img.IsOk())
                { m_pOwner->SetBitmap(img); }
            else
                { return false; }
            }
        return true;
        }

    //----------------------------------
    Thumbnail::Thumbnail(wxWindow* parent, const wxBitmap& bmp,
                         const ClickMode clickMode /*= FullSizeViewable*/,
                         const bool allowFileDrop /*= false*/, wxWindowID id /*= wxID_ANY*/,
                         const wxPoint& pos /*= wxDefaultPosition*/,
                         const wxSize& size /*= wxDefaultSize*/,
                         long style /*= wxFULL_REPAINT_ON_RESIZE|wxBORDER_SIMPLE*/,
                         const wxString& name /*= L"Thumbnail"*/) :
            wxWindow(parent, id, pos, wxDefaultSize, style, name),
            m_img(bmp.ConvertToImage()),
            m_clickMode(clickMode),
            m_opactity(wxALPHA_OPAQUE)
        {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        if (!size.IsFullySpecified())
            {
            SetSize(FromDIP(wxSize(64, 64)));
            SetMinSize(FromDIP(wxSize(64, 64)));
            }
        else
            {
            SetSize(FromDIP(size));
            SetMinSize(FromDIP(size));
            }
        const wxSize newSize = m_img.SetBestSize(GetSize());
        SetSize(newSize);
        SetMinSize(newSize);

        // if original image is smaller or same size as this control,
        // then no reason to have "click to view" support
        if ((m_clickMode == ClickMode::FullSizeViewable) && bmp.IsOk() &&
            (bmp.GetSize().GetWidth() > GetSize().GetWidth() ||
             bmp.GetSize().GetHeight() > GetSize().GetHeight()))
            {
            SetCursor(wxCURSOR_HAND);
            SetToolTip(_(L"Click to enlarge..."));
            }
        else if (m_clickMode == ClickMode::BrowseForImageFile)
            {
            SetCursor(wxCURSOR_HAND);
            SetToolTip(_(L"Click to browse for image..."));
            }
        if (allowFileDrop)
            {
            DragAcceptFiles(true);
            SetDropTarget(new DropThumbnailImageFile(this));
            }

        Bind(wxEVT_SIZE, &Thumbnail::OnResize, this);
        Bind(wxEVT_LEFT_DOWN, &Thumbnail::OnClick, this);
        Bind(wxEVT_PAINT, &Thumbnail::OnPaint, this);

        Refresh();
        Update();
        }

    //----------------------------------
    void Thumbnail::SetBitmap(const wxBitmap& bmp)
        {
        m_img = GraphItems::Image(bmp.ConvertToImage());
        const wxSize newSize = m_img.SetBestSize(GetSize());
        SetSize(newSize);
        SetMinSize(newSize);
        Refresh();
        Update();
        }

    //----------------------------------
    bool Thumbnail::LoadImage(const wxString& filePath)
        {
        m_img = GraphItems::Image(GraphItems::Image::LoadFile(filePath));
        const wxSize newSize = m_img.SetBestSize(GetSize());
        SetSize(newSize);
        SetMinSize(newSize);
        Refresh();
        Update();
        return (m_img.IsOk());
        }

    //----------------------------------
    void Thumbnail::OnPaint([[maybe_unused]] wxPaintEvent& event)
        {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(GetParent()->GetBackgroundColour());
        dc.Clear();
        if (m_img.IsOk())
            {
            m_img.SetDPIScaleFactor(GetDPIScaleFactor());
            m_img.SetOpacity(GetOpacity());
            m_img.SetAnchoring(Anchoring::TopLeftCorner);
            m_img.SetAnchorPoint(
                wxPoint(safe_divide(GetSize().GetWidth() -
                                    m_img.GetImageSize().GetWidth(), 2),
                        safe_divide(GetSize().GetHeight() -
                                    m_img.GetImageSize().GetHeight(), 2)));
            m_img.Draw(dc);
            }
        else
            {
            if (m_clickMode == ClickMode::BrowseForImageFile)
                {
                const wxString label =_(L"Click to browse\nfor image...");
                wxCoord textWidth, textHeight;
                dc.GetMultiLineTextExtent(label, &textWidth, &textHeight);
                dc.DrawText(label, safe_divide(GetSize().GetWidth(),2) - safe_divide(textWidth,2),
                    safe_divide(GetSize().GetHeight(),2)-safe_divide(textHeight,2));
                }
            else
                {
                const wxString label = _(L"No preview\navailable");
                wxCoord textWidth, textHeight;
                dc.GetMultiLineTextExtent(label, &textWidth, &textHeight);
                dc.DrawText(label, safe_divide(GetSize().GetWidth(),2) - safe_divide(textWidth,2),
                    safe_divide(GetSize().GetHeight(),2)-safe_divide(textHeight,2));
                }
            }
        }

    //----------------------------------
    void Thumbnail::OnResize(wxSizeEvent& event)
        {
        m_img.SetSize(event.GetSize());
        Refresh();
        Update();
        event.Skip();
        }

    //----------------------------------
    void Thumbnail::OnClick([[maybe_unused]] wxMouseEvent& event)
        {
        if ((m_clickMode == ClickMode::FullSizeViewable) && m_img.IsOk())
            {
            // if original image is smaller or same size as this control,
            // then no reason to have click to view support
            if (m_img.GetOriginalImage().GetWidth() <= GetSize().GetWidth() &&
                m_img.GetOriginalImage().GetHeight() <= GetSize().GetHeight())
                { return; }
            // rescale image if caller requested a size different from the original bitmap's size
            wxBitmap canvasBmp{ m_img.GetOriginalImage() };

            const std::pair<double,double> scaledSize =
                geometry::downscaled_size(
                    std::make_pair<double, double>(canvasBmp.GetWidth(),canvasBmp.GetHeight()),
                    std::make_pair<double, double>(wxSystemSettings::GetMetric(wxSYS_SCREEN_X),
                                                   wxSystemSettings::GetMetric(wxSYS_SCREEN_Y)));
            if (wxSize(scaledSize.first, scaledSize.second) != canvasBmp.GetSize())
                {
                canvasBmp = wxBitmap(canvasBmp.ConvertToImage().
                    Rescale(scaledSize.first, scaledSize.second, wxIMAGE_QUALITY_HIGH));
                }
            // add a little "click to close" label at the bottom of the full image
            wxMemoryDC memDC(canvasBmp);
            memDC.SetFont(wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize(),
                                 wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"Times New Roman"));
            memDC.SetTextForeground(*wxBLUE);
            memDC.SetPen(*wxBLACK_PEN);
            memDC.SetBrush(wxBrush(wxColour(L"#FFFFDD")));
            wxCoord width, height;
            const wxString label(_(L"Click to close"));
            memDC.GetTextExtent(label, &width, &height);
            memDC.DrawRoundedRectangle(canvasBmp.GetWidth()-(width + 14),
                                       canvasBmp.GetHeight()-(height + 14),
                                       width+8, height + 8, 2);
            memDC.DrawText(label, canvasBmp.GetWidth()-(width + 10),
                           canvasBmp.GetHeight()-(height + 10));
            // draw a border around the image (some platforms don't put a border around dialogs)
            memDC.SetPen(*wxBLACK_PEN);
                memDC.DrawLine(0, 0, memDC.GetSize().GetWidth(), 0);
                memDC.DrawLine(0, memDC.GetSize().GetHeight() - 1,
                               memDC.GetSize().GetWidth(),memDC.GetSize().GetHeight()-1);
                memDC.DrawLine(0, 0, 0, memDC.GetSize().GetHeight());
                memDC.DrawLine(memDC.GetSize().GetWidth() - 1, 0,
                               memDC.GetSize().GetWidth() - 1, memDC.GetSize().GetHeight());
            memDC.SelectObject(wxNullBitmap);

            EnlargedImageWindow enlargedImage(canvasBmp, this);
            // would be nice to show with wxSHOW_EFFECT_EXPAND, but it looks really bad on Windows
            enlargedImage.ShowModal();
            }
        else if (m_clickMode == ClickMode::BrowseForImageFile)
            {
            wxFileDialog fileDlg(this, _(L"Select an Image"), wxString{}, wxString{},
                          _(L"Image Files ") + wxImage::GetImageExtWildcard(),
                          wxFD_OPEN|wxFD_PREVIEW);
            if (fileDlg.ShowModal() == wxID_OK)
                { SetBitmap(wxBitmap(GraphItems::Image::LoadFile(fileDlg.GetPath()))); }
            }
        }
    }