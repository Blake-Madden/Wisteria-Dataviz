///////////////////////////////////////////////////////////////////////////////
// Name:        htmltablewin.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmltablewin.h"
#include "../../import/html_encode.h"
#include "../../import/html_extract_text.h"
#include "htmltablewinprintout.h"
#include <wx/print.h>

using namespace Wisteria::UI;

wxIMPLEMENT_DYNAMIC_CLASS(HtmlTableWindow, wxHtmlWindow)

    //------------------------------------------------------
    HtmlTableWindow::HtmlTableWindow(
        wxWindow* parent, const wxWindowID id /*= wxID_ANY*/,
        const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/,
        const long style /*= wxHW_DEFAULT_STYLE | wxHW_NO_SELECTION | wxBORDER_THEME*/)
    : wxHtmlWindow(parent, id, pos, size, style, L"HtmlTableWindow")
    {
    m_menu.Append(wxID_COPY, _(L"Copy"));
    m_menu.AppendSeparator();
    m_menu.Append(wxID_PRINT, _(L"Print"));
    // Save button can be disabled by docmanger if parent document is not dirty,
    // but here save is just meant for this window.
    m_menu.Append(XRCID("ID_SAVE_ITEM"), _(L"Save"));

    Bind(wxEVT_FIND, &HtmlTableWindow::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &HtmlTableWindow::OnFind, this);
    Bind(wxEVT_FIND_CLOSE, &HtmlTableWindow::OnFind, this);

    Bind(wxEVT_RIGHT_DOWN, &HtmlTableWindow::OnRightClick, this);
    // standard menu commands
    Bind(wxEVT_MENU, &HtmlTableWindow::OnCopy, this, wxID_COPY);
    Bind(wxEVT_MENU, &HtmlTableWindow::OnPreview, this, wxID_PREVIEW);
    Bind(wxEVT_MENU, &HtmlTableWindow::OnPrint, this, wxID_PRINT);
    Bind(wxEVT_MENU, &HtmlTableWindow::OnSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &HtmlTableWindow::OnSave, this, XRCID("ID_SAVE_ITEM"));
    // button variations of above commands
    Bind(wxEVT_BUTTON, &HtmlTableWindow::OnCopy, this, wxID_COPY);
    Bind(wxEVT_BUTTON, &HtmlTableWindow::OnPreview, this, wxID_PREVIEW);
    Bind(wxEVT_BUTTON, &HtmlTableWindow::OnPrint, this, wxID_PRINT);
    Bind(wxEVT_BUTTON, &HtmlTableWindow::OnSave, this, wxID_SAVE);
    Bind(wxEVT_BUTTON, &HtmlTableWindow::OnSave, this, XRCID("ID_SAVE_ITEM"));
    }

//------------------------------------------------------
void HtmlTableWindow::OnPrint([[maybe_unused]] wxCommandEvent& event)
    {
    HtmlTablePrintout* printOut = new HtmlTablePrintout(GetLabel());
    printOut->SetDPIScaleFactor(GetDPIScaleFactor());
    printOut->SetLeftPrinterHeader(GetLeftPrinterHeader());
    printOut->SetCenterPrinterHeader(GetCenterPrinterHeader());
    printOut->SetRightPrinterHeader(GetRightPrinterHeader());
    printOut->SetLeftPrinterFooter(GetLeftPrinterFooter());
    printOut->SetCenterPrinterFooter(GetCenterPrinterFooter());
    printOut->SetRightPrinterFooter(GetRightPrinterFooter());
    printOut->SetWatermark(GetWatermark());

    wxString htmlText = *(GetParser()->GetSource());
    const wchar_t* htmlEnd = htmlText.wc_str() + htmlText.length();
    const wchar_t* tableStart = lily_of_the_valley::html_extract_text::find_element(
        htmlText.wc_str(), htmlEnd, _DT(L"table"), false);
    const wchar_t* tableEnd = nullptr;
    while (tableStart)
        {
        tableEnd = lily_of_the_valley::html_extract_text::find_closing_element(tableStart, htmlEnd,
                                                                               _DT(L"table"));
        if (tableEnd == nullptr)
            {
            break;
            }
        else
            {
            tableEnd = lily_of_the_valley::html_extract_text::find_close_tag(tableEnd);
            if (tableEnd == nullptr)
                {
                break;
                }
            // skip '>'
            ++tableEnd;
            printOut->AddTable(wxString(tableStart, tableEnd - tableStart));
            }
        tableStart = lily_of_the_valley::html_extract_text::find_element(tableEnd, htmlEnd,
                                                                         _DT(L"table"), false);
        }

#if defined(__WXMSW__) || defined(__WXOSX__)
    wxPrinterDC* dc = nullptr;
#else
    wxPostScriptDC* dc = nullptr;
#endif
    if (m_printData)
        {
#if defined(__WXMSW__) || defined(__WXOSX__)
        dc = new wxPrinterDC(*m_printData);
#else
        dc = new wxPostScriptDC(*m_printData);
#endif
        }
    else
        {
        wxPrintData pd;
#if defined(__WXMSW__) || defined(__WXOSX__)
        dc = new wxPrinterDC(pd);
#else
        dc = new wxPostScriptDC(pd);
#endif
        }
    printOut->SetDC(dc);

    wxPrinter printer;
    if (m_printData)
        {
        printer.GetPrintDialogData().SetPrintData(*m_printData);
        }
    printer.GetPrintDialogData().SetAllPages(true);
    printer.GetPrintDialogData().SetFromPage(1);
    printer.GetPrintDialogData().SetMinPage(1);
    printer.GetPrintDialogData().EnableSelection(false);
    if (!printer.Print(this, printOut, true))
        {
        // just show a message if a real error occurred. They may have just cancelled.
        if (printer.GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print"), wxOK | wxICON_WARNING);
            }
        }
    if (m_printData)
        {
        *m_printData = printer.GetPrintDialogData().GetPrintData();
        }
    wxDELETE(printOut);
    wxDELETE(dc);
    }

//------------------------------------------------------
void HtmlTableWindow::OnPreview([[maybe_unused]] wxCommandEvent& event)
    {
#if defined(__WXMSW__)
    HtmlTablePrintout* printOut = new HtmlTablePrintout(GetLabel());
    printOut->SetDPIScaleFactor(GetDPIScaleFactor());
    printOut->SetLeftPrinterHeader(GetLeftPrinterHeader());
    printOut->SetCenterPrinterHeader(GetCenterPrinterHeader());
    printOut->SetRightPrinterHeader(GetRightPrinterHeader());
    printOut->SetLeftPrinterFooter(GetLeftPrinterFooter());
    printOut->SetCenterPrinterFooter(GetCenterPrinterFooter());
    printOut->SetRightPrinterFooter(GetRightPrinterFooter());
    printOut->SetWatermark(GetWatermark());

    HtmlTablePrintout* printOutForPrinting = new HtmlTablePrintout(GetLabel());
    printOutForPrinting->SetDPIScaleFactor(GetDPIScaleFactor());
    printOutForPrinting->SetLeftPrinterHeader(GetLeftPrinterHeader());
    printOutForPrinting->SetCenterPrinterHeader(GetCenterPrinterHeader());
    printOutForPrinting->SetRightPrinterHeader(GetRightPrinterHeader());
    printOutForPrinting->SetLeftPrinterFooter(GetLeftPrinterFooter());
    printOutForPrinting->SetCenterPrinterFooter(GetCenterPrinterFooter());
    printOutForPrinting->SetRightPrinterFooter(GetRightPrinterFooter());
    printOutForPrinting->SetWatermark(GetWatermark());

    wxString htmlText = *(GetParser()->GetSource());
    const wchar_t* htmlEnd = htmlText.wc_str() + htmlText.length();
    const wchar_t* tableStart = lily_of_the_valley::html_extract_text::find_element(
        htmlText.wc_str(), htmlEnd, _DT(L"table"), false);
    const wchar_t* tableEnd = nullptr;
    while (tableStart)
        {
        tableEnd = lily_of_the_valley::html_extract_text::find_closing_element(tableStart, htmlEnd,
                                                                               _DT(L"table"));
        if (tableEnd == nullptr)
            {
            break;
            }
        else
            {
            tableEnd = lily_of_the_valley::html_extract_text::find_close_tag(tableEnd);
            if (tableEnd == nullptr)
                {
                break;
                }
            // skip '>'
            ++tableEnd;
            printOut->AddTable(wxString(tableStart, tableEnd - tableStart));
            printOutForPrinting->AddTable(wxString(tableStart, tableEnd - tableStart));
            }
        tableStart = lily_of_the_valley::html_extract_text::find_element(tableEnd, htmlEnd,
                                                                         _DT(L"table"), false);
        }
    wxPrinterDC* dc = nullptr;
    wxPrinterDC* dc2 = nullptr;
    if (m_printData)
        {
        dc = new wxPrinterDC(*m_printData);
        dc2 = new wxPrinterDC(*m_printData);
        }
    else
        {
        wxPrintData pd;
        dc = new wxPrinterDC(pd);
        dc2 = new wxPrinterDC(pd);
        }
    printOut->SetDC(dc);
    printOutForPrinting->SetDC(dc2);

    wxPrintPreview* preview = new wxPrintPreview(printOut, printOutForPrinting, m_printData);
    preview->GetPrintDialogData().SetAllPages(true);
    preview->GetPrintDialogData().SetFromPage(1);
    preview->GetPrintDialogData().SetMinPage(1);
    preview->GetPrintDialogData().EnableSelection(false);
    if (!preview->IsOk())
        {
        wxDELETE(preview);
        wxDELETE(dc);
        wxDELETE(dc2);
        wxMessageBox(_(L"An error occurred while previewing.\n"
                       "Your default printer may not be set correctly."),
                     _(L"Print Preview"), wxOK | wxICON_WARNING);
        return;
        }
    int x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };
    wxClientDisplayRect(&x, &y, &width, &height);
    wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                               wxDefaultPosition, wxSize(width, height));

    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);

    delete dc;
    delete dc2;
#else
    wxFAIL_MSG(L"Print preview is Windows only!");
#endif
    }

//------------------------------------------------------
void HtmlTableWindow::OnSave([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog dialog(this, _(L"Save As"), wxEmptyString, GetLabel(),
                        L"HTML (*.htm;*.html)|*.htm;*.html", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK)
        {
        return;
        }

    wxFileName filePath = dialog.GetPath();
    // in case the extension is missing then use the selected filter
    if (filePath.GetExt().empty())
        {
        filePath.SetExt(L"htm");
        }

    Save(filePath);
    }

//------------------------------------------------------
bool HtmlTableWindow::Save(const wxFileName& path) const
    {
    // create the folder to the filepath, if necessary
    wxFileName::Mkdir(path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    wxASSERT(GetParser());
    std::wstring htmlText{ (GetParser()->GetSource())->wc_str() };
    lily_of_the_valley::html_format::strip_body_attributes(htmlText);
    lily_of_the_valley::html_format::strip_hyperlinks(htmlText);
    lily_of_the_valley::html_format::set_title(htmlText, GetLabel().ToStdWstring());
    lily_of_the_valley::html_format::set_encoding(htmlText);
    const bool retVal = file.Write(htmlText);
    if (!retVal)
        {
        wxMessageBox(wxString::Format(_(L"Failed to save document\n(%s)."), path.GetFullPath()),
                     _(L"Error"), wxOK | wxICON_EXCLAMATION);
        }
    return retVal;
    }

//------------------------------------------------------
void HtmlTableWindow::Copy()
    {
    wxASSERT(GetParser());
    if (wxTheClipboard->Open())
        {
        wxTheClipboard->Clear();
        auto* obj = new wxDataObjectComposite();
        std::wstring htmlText{ (GetParser()->GetSource())->wc_str() };
        lily_of_the_valley::html_format::strip_hyperlinks(htmlText);
        lily_of_the_valley::html_format::strip_images(htmlText);
        lily_of_the_valley::html_format::set_title(htmlText, GetLabel().ToStdWstring());
        lily_of_the_valley::html_format::set_encoding(htmlText);
        obj->Add(new wxHTMLDataObject(htmlText), true);
        obj->Add(new wxTextDataObject(ToText()));
        wxTheClipboard->SetData(obj);
        wxTheClipboard->Close();
        }
    }

//------------------------------------------------------
void HtmlTableWindow::OnSelectAll([[maybe_unused]] wxCommandEvent& event) { SelectAll(); }

//------------------------------------------------------
void HtmlTableWindow::OnCopy([[maybe_unused]] wxCommandEvent& event) { Copy(); }

//------------------------------------------------------
void HtmlTableWindow::OnRightClick([[maybe_unused]] wxMouseEvent& event) { PopupMenu(&m_menu); }

//------------------------------------------------------
void HtmlTableWindow::OnFind([[maybe_unused]] wxFindDialogEvent& event)
    {
    wxMessageBox(_(L"Find not supported in this window."), _(L"Find"), wxOK | wxICON_INFORMATION);
    }
