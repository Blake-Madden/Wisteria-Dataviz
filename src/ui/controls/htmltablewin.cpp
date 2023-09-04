///////////////////////////////////////////////////////////////////////////////
// Name:        htmltablewin.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmltablewin.h"
#include "htmltablewinprintout.h"
#include "../../import/html_extract_text.h"
#include "../../import/html_encode.h"

using namespace Wisteria::UI;

wxIMPLEMENT_DYNAMIC_CLASS(HtmlTableWindow, wxHtmlWindow)

wxBEGIN_EVENT_TABLE(HtmlTableWindow, wxHtmlWindow)
    EVT_FIND(wxID_ANY, HtmlTableWindow::OnFind)
    EVT_FIND_NEXT(wxID_ANY, HtmlTableWindow::OnFind)
    EVT_FIND_CLOSE(wxID_ANY, HtmlTableWindow::OnFind)
    EVT_RIGHT_DOWN(HtmlTableWindow::OnRightClick)
    // standard menu commands
    EVT_MENU(wxID_SELECTALL, HtmlTableWindow::OnSelectAll)
    EVT_MENU(wxID_COPY, HtmlTableWindow::OnCopy)
    EVT_MENU(XRCID("ID_COPY_ALL"), HtmlTableWindow::OnCopyAll)
    EVT_MENU(wxID_PREVIEW, HtmlTableWindow::OnPreview)
    EVT_MENU(wxID_PRINT, HtmlTableWindow::OnPrint)
    EVT_MENU(wxID_SAVE, HtmlTableWindow::OnSave)
    // button variations of above commands
    EVT_BUTTON(wxID_SELECTALL, HtmlTableWindow::OnSelectAll)
    EVT_BUTTON(wxID_COPY, HtmlTableWindow::OnCopy)
    EVT_BUTTON(XRCID("ID_COPY_ALL"), HtmlTableWindow::OnCopyAll)
    EVT_BUTTON(wxID_PREVIEW, HtmlTableWindow::OnPreview)
    EVT_BUTTON(wxID_PRINT, HtmlTableWindow::OnPrint)
    EVT_BUTTON(wxID_SAVE, HtmlTableWindow::OnSave)
wxEND_EVENT_TABLE()

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
    const wchar_t* htmlEnd = htmlText.wc_str()+htmlText.length();
    const wchar_t* tableStart =
        lily_of_the_valley::html_extract_text::find_element(htmlText.wc_str(), htmlEnd,
                                                            _DT(L"table"), 5, false);
    const wchar_t* tableEnd = nullptr;
    while (tableStart)
        {
        tableEnd =
            lily_of_the_valley::html_extract_text::find_closing_element(tableStart, htmlEnd,
                                                                        _DT(L"table"), 5);
        if (tableEnd == nullptr)
            { break; }
        else
            {
            tableEnd = lily_of_the_valley::html_extract_text::find_close_tag(tableEnd);
            if (tableEnd == nullptr)
                { break; }
            // skip '>'
            ++tableEnd;
            printOut->AddTable(wxString(tableStart, tableEnd-tableStart));
            }
        tableStart =
            lily_of_the_valley::html_extract_text::find_element(tableEnd, htmlEnd,
                                                                _DT(L"table"), 5, false);
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
    if (!printer.Print(this, printOut, true) )
        {
        // just show a message if a real error occurred. They may have just cancelled.
        if (printer.GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                            "Your default printer may not be set correctly."),
                         _(L"Print"), wxOK|wxICON_WARNING);
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
    const wchar_t* htmlEnd = htmlText.wc_str()+htmlText.length();
    const wchar_t* tableStart =
        lily_of_the_valley::html_extract_text::find_element(htmlText.wc_str(), htmlEnd,
                                                            _DT(L"table"), 5, false);
    const wchar_t* tableEnd = nullptr;
    while (tableStart)
        {
        tableEnd =
            lily_of_the_valley::html_extract_text::find_closing_element(tableStart, htmlEnd,
                                                                        _DT(L"table"), 5);
        if (tableEnd == nullptr)
            { break; }
        else
            {
            tableEnd = lily_of_the_valley::html_extract_text::find_close_tag(tableEnd);
            if (tableEnd == nullptr)
                { break; }
            // skip '>'
            ++tableEnd;
            printOut->AddTable(wxString(tableStart, tableEnd-tableStart));
            printOutForPrinting->AddTable(wxString(tableStart, tableEnd-tableStart));
            }
        tableStart =
            lily_of_the_valley::html_extract_text::find_element(tableEnd, htmlEnd,
                                                                _DT(L"table"), 5, false);
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
    if (!preview->IsOk())
        {
        wxDELETE(preview); wxDELETE(dc); wxDELETE(dc2);
        wxMessageBox(_(L"An error occurred while previewing.\n"
                        "Your default printer may not be set correctly."),
                     _(L"Print Preview"), wxOK|wxICON_WARNING);
        return;
        }
    int x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };
    wxClientDisplayRect(&x, &y, &width, &height);
    wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                                wxDefaultPosition, wxSize(width, height));

    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);

    delete dc; delete dc2;
#else
    wxFAIL_MSG(L"Print preview is Windows only!");
#endif
    }

//------------------------------------------------------
void HtmlTableWindow::OnSave([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog dialog
            (
            this,
            _(L"Save As"),
            wxEmptyString,
            GetLabel(),
            L"HTML (*.htm;*.html)|*.htm;*.html",
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK)
        { return; }

    wxFileName filePath = dialog.GetPath();
    // in case the extension is missing then use the selected filter
    if (filePath.GetExt().empty())
        { filePath.SetExt(L"htm"); }

    Save(filePath);
    }

//------------------------------------------------------
bool HtmlTableWindow::Save(const wxFileName& path)
    {
    // create the folder to the filepath, if necessary
    wxFileName::Mkdir(path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    assert(GetParser());
    std::wstring htmlText{ (GetParser()->GetSource())->wc_str() };
    lily_of_the_valley::html_format::strip_body_atributes(htmlText);
    lily_of_the_valley::html_format::strip_hyperlinks(htmlText);
    lily_of_the_valley::html_format::set_title(htmlText, GetLabel().ToStdWstring());
    lily_of_the_valley::html_format::set_encoding(htmlText);
    const bool retVal = file.Write(htmlText);
    if (!retVal)
        {
        wxMessageBox(wxString::Format(_(L"Failed to save document\n(%s)."),
            path.GetFullPath()),
            _(L"Error"), wxOK|wxICON_EXCLAMATION);
        }
    return retVal;
    }

//------------------------------------------------------
void HtmlTableWindow::OnSelectAll([[maybe_unused]] wxCommandEvent& event )
    { SelectAll(); }

//------------------------------------------------------
void HtmlTableWindow::OnCopy([[maybe_unused]] wxCommandEvent& event )
    {
    if (wxTheClipboard->Open())
        {
        if (wxTheClipboard->SetData(new wxTextDataObject(SelectionToText()) ) )
            { wxTheClipboard->Close(); }
        }
    }

//------------------------------------------------------
void HtmlTableWindow::OnCopyAll([[maybe_unused]] wxCommandEvent& event )
    {
    if (wxTheClipboard->Open())
        {
        wxTheClipboard->Clear();
        wxDataObjectComposite* obj = new wxDataObjectComposite();
        std::wstring htmlText{ (GetParser()->GetSource())->wc_str() };
        lily_of_the_valley::html_format::strip_hyperlinks(htmlText);
        lily_of_the_valley::html_format::strip_images(htmlText);
        lily_of_the_valley::html_format::set_title(htmlText, GetLabel().ToStdWstring());
        lily_of_the_valley::html_format::set_encoding(htmlText);
        obj->Add(new wxHTMLDataObject(htmlText), true);
        obj->Add(new wxTextDataObject(ToText()) );
        wxTheClipboard->AddData(obj);
        wxTheClipboard->Close();
        }
    }

//------------------------------------------------------
void HtmlTableWindow::OnRightClick([[maybe_unused]] wxMouseEvent& event)
    {
    if (m_menu == nullptr)
        { return; }
    // if nothing selected then disable the Copy option
    if (SelectionToText().empty() )
        { m_menu->Enable(wxID_COPY, false); }
    else
        { m_menu->Enable(wxID_COPY, true); }
    PopupMenu(m_menu);
    }

//------------------------------------------------------
void HtmlTableWindow::OnFind([[maybe_unused]] wxFindDialogEvent& event)
    {
    wxMessageBox(_(L"Find not supported in this window."),
            _(L"Find"), wxOK|wxICON_INFORMATION);
    }
