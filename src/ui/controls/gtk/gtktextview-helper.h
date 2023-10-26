/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
            Tim-Philipp Mueller (Gtk+ bug report patch for GtkTextView Pango-loading functionality)
            Anthony Bretaudeau (portions of wxWidgets printing code)
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License (some portions are wxWindows licensed, where noted).

     SPDX-License-Identifier: BSD-3-Clause
     SPDX-License-Identifier: wxWindows
@{*/

#ifndef __GTK_TEXTVIEW_PANGO_MARKUP__
#define __GTK_TEXTVIEW_PANGO_MARKUP__

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gtk/gtktextiter.h>
#include <gtk/gtktexttag.h>
#include <gtk/gtktextchild.h>
#include <gtk/gtktextiter.h>
#include <cstring>
#include <string>
#include <vector>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/paper.h>
#include <wx/gtk/print.h>

// Printing system for GtkTextView
//--------------------------------

// From wxWidget's gtk/print.cpp:
// Map wxPaperSize to GtkPaperSize names
// Ordering must be the same as wxPaperSize enum
static const char* const gtk_paperList[] =
    {
    nullptr, // wxPAPER_NONE
    "na_letter", // wxPAPER_LETTER
    "na_legal", // wxPAPER_LEGAL
    "iso_a4", // wxPAPER_A4
    "na_c", // wxPAPER_CSHEET
    "na_d", // wxPAPER_DSHEET
    "na_e", // wxPAPER_ESHEET
    "na_letter", // wxPAPER_LETTERSMALL
    "na_ledger", // wxPAPER_TABLOID
    "na_ledger", // wxPAPER_LEDGER
    "na_invoice", // wxPAPER_STATEMENT
    "na_executive", // wxPAPER_EXECUTIVE
    "iso_a3", // wxPAPER_A3
    "iso_a4", // wxPAPER_A4SMALL
    "iso_a5", // wxPAPER_A5
    "jis_b4", // wxPAPER_B4 "B4 (JIS) 257 x 364 mm"
    "jis_b5", // wxPAPER_B5 "B5 (JIS) 182 x 257 mm"
    "om_folio", // wxPAPER_FOLIO
    "na_quarto", // wxPAPER_QUARTO
    "na_10x14", // wxPAPER_10X14
    "na_ledger", // wxPAPER_11X17
    "na_letter", // wxPAPER_NOTE
    "na_number-9", // wxPAPER_ENV_9
    "na_number-10", // wxPAPER_ENV_10
    "na_number-11", // wxPAPER_ENV_11
    "na_number-12", // wxPAPER_ENV_12
    "na_number-14", // wxPAPER_ENV_14
    "iso_dl", // wxPAPER_ENV_DL
    "iso_c5", // wxPAPER_ENV_C5
    "iso_c3", // wxPAPER_ENV_C3
    "iso_c4", // wxPAPER_ENV_C4
    "iso_c6", // wxPAPER_ENV_C6
    "iso_c6c5", // wxPAPER_ENV_C65
    "iso_b4", // wxPAPER_ENV_B4
    "iso_b5", // wxPAPER_ENV_B5
    "iso_b6", // wxPAPER_ENV_B6
    "om_italian", // wxPAPER_ENV_ITALY
    "na_monarch", // wxPAPER_ENV_MONARCH
    "na_personal", // wxPAPER_ENV_PERSONAL
    "na_fanfold-us", // wxPAPER_FANFOLD_US
    "na_fanfold-eur", // wxPAPER_FANFOLD_STD_GERMAN
    "na_foolscap", // wxPAPER_FANFOLD_LGL_GERMAN
    "iso_b4", // wxPAPER_ISO_B4
    "jpn_hagaki", // wxPAPER_JAPANESE_POSTCARD
    "na_9x11", // wxPAPER_9X11
    "na_10x11", // wxPAPER_10X11
    "na_11x15", // wxPAPER_15X11
    "om_invite", // wxPAPER_ENV_INVITE
    "na_letter-extra", // wxPAPER_LETTER_EXTRA
    "na_legal-extra", // wxPAPER_LEGAL_EXTRA
    "na_arch-b", // wxPAPER_TABLOID_EXTRA
    "iso_a4-extra", // wxPAPER_A4_EXTRA
    "na_letter", // wxPAPER_LETTER_TRANSVERSE
    "iso_a4", // wxPAPER_A4_TRANSVERSE
    "na_letter-extra", // wxPAPER_LETTER_EXTRA_TRANSVERSE
    "na_super-a", // wxPAPER_A_PLUS
    "na_super-b", // wxPAPER_B_PLUS
    "na_letter-plus", // wxPAPER_LETTER_PLUS
    "om_folio", // wxPAPER_A4_PLUS "A4 Plus 210 x 330 mm" (no A4 Plus in PWG standard)
    "iso_a5", // wxPAPER_A5_TRANSVERSE
    "jis_b5", // wxPAPER_B5_TRANSVERSE "B5 (JIS) Transverse 182 x 257 mm"
    "iso_a3-extra", // wxPAPER_A3_EXTRA
    "iso_a5-extra", // wxPAPER_A5_EXTRA
    "iso_b5-extra", // wxPAPER_B5_EXTRA
    "iso_a2", // wxPAPER_A2
    "iso_a3", // wxPAPER_A3_TRANSVERSE
    "iso_a3-extra", // wxPAPER_A3_EXTRA_TRANSVERSE
    "jpn_oufuku", // wxPAPER_DBL_JAPANESE_POSTCARD
    "iso_a6", // wxPAPER_A6
    "jpn_kaku2", // wxPAPER_JENV_KAKU2
    "jpn_kaku3_216x277mm", // wxPAPER_JENV_KAKU3
    "jpn_chou3", // wxPAPER_JENV_CHOU3
    "jpn_chou4", // wxPAPER_JENV_CHOU4
    "na_letter", // wxPAPER_LETTER_ROTATED
    "iso_a3", // wxPAPER_A3_ROTATED
    "iso_a4", // wxPAPER_A4_ROTATED
    "iso_a5", // wxPAPER_A5_ROTATED
    "jis_b4", // wxPAPER_B4_JIS_ROTATED
    "jis_b5", // wxPAPER_B5_JIS_ROTATED
    "jpn_hagaki", // wxPAPER_JAPANESE_POSTCARD_ROTATED
    "jpn_oufuku", // wxPAPER_DBL_JAPANESE_POSTCARD_ROTATED
    "iso_a6", // wxPAPER_A6_ROTATED
    "jpn_kaku2", // wxPAPER_JENV_KAKU2_ROTATED
    "jpn_kaku3_216x277mm", // wxPAPER_JENV_KAKU3_ROTATED
    "jpn_chou3", // wxPAPER_JENV_CHOU3_ROTATED
    "jpn_chou4", // wxPAPER_JENV_CHOU4_ROTATED
    "jis_b6", // wxPAPER_B6_JIS
    "jis_b6", // wxPAPER_B6_JIS_ROTATED
    "na_11x12", // wxPAPER_12X11
    "jpn_you4", // wxPAPER_JENV_YOU4
    "jpn_you4", // wxPAPER_JENV_YOU4_ROTATED
    "prc_16k", // wxPAPER_P16K
    "prc_32k", // wxPAPER_P32K
    "prc_32k", // wxPAPER_P32KBIG
    "prc_1", // wxPAPER_PENV_1
    "prc_2", // wxPAPER_PENV_2
    "prc_3", // wxPAPER_PENV_3
    "prc_4", // wxPAPER_PENV_4
    "prc_5", // wxPAPER_PENV_5
    "prc_6", // wxPAPER_PENV_6
    "prc_7", // wxPAPER_PENV_7
    "prc_8", // wxPAPER_PENV_8
    "prc_9", // wxPAPER_PENV_9
    "prc_10", // wxPAPER_PENV_10
    "prc_16k", // wxPAPER_P16K_ROTATED
    "prc_32k", // wxPAPER_P32K_ROTATED
    "prc_32k", // wxPAPER_P32KBIG_ROTATED
    "prc_1", // wxPAPER_PENV_1_ROTATED
    "prc_2", // wxPAPER_PENV_2_ROTATED
    "prc_3", // wxPAPER_PENV_3_ROTATED
    "prc_4", // wxPAPER_PENV_4_ROTATED
    "prc_5", // wxPAPER_PENV_5_ROTATED
    "prc_6", // wxPAPER_PENV_6_ROTATED
    "prc_7", // wxPAPER_PENV_7_ROTATED
    "prc_8", // wxPAPER_PENV_8_ROTATED
    "prc_9", // wxPAPER_PENV_9_ROTATED
    "prc_10", // wxPAPER_PENV_10_ROTATED
    "iso_a0", // wxPAPER_A0
    "iso_a1"  // wxPAPER_A1
    };

struct _GtkPageLines
    {
    gint m_page{ 0 };
    GSList* m_linesStart{ nullptr };
    gint m_numberOfLines{ 0 };
    };

struct _GtkPrintData
    {
    std::string m_markupContent;
    PangoLayout* m_layout{ nullptr };
    GSList* m_lines{ nullptr };
    std::vector<_GtkPageLines> m_pageLines;
    gint m_headerAreaHeight{ 0 };
    gint m_footerAreaHeight{ 0 };
    wxString m_leftPrintHeader;
    wxString m_centerPrintHeader;
    wxString m_rightPrintHeader;
    wxString m_leftPrintFooter;
    wxString m_centerPrintFooter;
    wxString m_rightPrintFooter;
    };

// GTK+ callbacks need C linkage
extern "C"
{
// GCC complains about these functions being unused, although we do use them as callbacks.
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

// Based on wxWidget's gtk/print.cpp:
//-------------------------------------------------
static GtkPaperSize* _GtkGetPaperSize(wxPaperSize paperId, const wxSize& size)
    {
    // if wxPaperSize is valid, get corresponding GtkPaperSize
    if (paperId > 0 && size_t(paperId) < std::size(gtk_paperList))
        { return gtk_paper_size_new(gtk_paperList[paperId]); }

    // if size is not valid, use a default GtkPaperSize
    if (size.x < 1 || size.y < 1)
        { return gtk_paper_size_new(gtk_paper_size_get_default()); }

    // look for a size match in GTK's GtkPaperSize list
    const double w{ static_cast<double>(size.x) };
    const double h{ static_cast<double>(size.y) };
    GtkPaperSize* paperSize{ nullptr };
    GList* list = gtk_paper_size_get_paper_sizes(TRUE);
    for (GList* p = list; p != nullptr; p = p->next)
        {
        GtkPaperSize* paperSize2 = static_cast<GtkPaperSize*>(p->data);
        if (paperSize == nullptr &&
            std::fabs(w - gtk_paper_size_get_width(paperSize2, GTK_UNIT_MM)) < 1 &&
            std::fabs(h - gtk_paper_size_get_height(paperSize2, GTK_UNIT_MM)) < 1)
            { paperSize = paperSize2; }
        else
            { gtk_paper_size_free(paperSize2); }
        }
    g_list_free(list);

    if (paperSize)
        { return paperSize; }

    // last resort, use a custom GtkPaperSize
    const wxString title = _("Custom size");
    const wxString name = wxString::Format("custom_%dx%d", size.x, size.y);
    return gtk_paper_size_new_custom(
        name.utf8_str(), title.utf8_str(), size.x, size.y, GTK_UNIT_MM);
    }

//-------------------------------------------------
static void _GtkUpdatePrintSettingsFromPageSetup(GtkPrintOperation* operation,
                                                 GtkPrintSettings* settings,
                                                 wxPrintData* printData)
    {
    // From wxWidget's gtk/print.cpp:
    // 
    // When embedding the page setup tab into the dialog, as we do, changes to
    // the settings such as the paper size and orientation there are not
    // reflected in the print settings, but must be retrieved from the page
    // setup struct itself separately.
    GtkPageSetup* defPageSetup{ nullptr };
    g_object_get(operation, "default-page-setup", &defPageSetup, nullptr);
    if (defPageSetup)
        {
        gtk_print_settings_set_orientation(settings, gtk_page_setup_get_orientation(defPageSetup));
        gtk_print_settings_set_paper_size(settings, gtk_page_setup_get_paper_size(defPageSetup));
        g_object_unref(defPageSetup);
        }

    printData->SetNoCopies(gtk_print_settings_get_n_copies(settings));
    printData->SetOrientation(
        (gtk_print_settings_get_orientation(settings) == GTK_PAGE_ORIENTATION_LANDSCAPE) ?
        wxLANDSCAPE : wxPORTRAIT);

    wxPaperSize paperId = wxPAPER_NONE;
    GtkPaperSize* pageSetupPaperSize = gtk_print_settings_get_paper_size(settings);
    if (pageSetupPaperSize != nullptr)
        {
        const char* name = gtk_paper_size_get_name(pageSetupPaperSize);
        for (size_t i = 1; i < std::size(gtk_paperList); ++i)
            {
            if (std::strcmp(name, gtk_paperList[i]) == 0)
                {
                paperId = static_cast<wxPaperSize>(i);
                break;
                }
            }
        if (paperId == wxPAPER_NONE)
            {
            // look for a size match in wxThePrintPaperDatabase
            const wxSize size(
                static_cast<int>(10 * gtk_paper_size_get_width(pageSetupPaperSize, GTK_UNIT_MM)),
                static_cast<int>(10 * gtk_paper_size_get_height(pageSetupPaperSize, GTK_UNIT_MM)));

            paperId = wxThePrintPaperDatabase->GetSize(size);

            // if no match, set custom size
            if (paperId == wxPAPER_NONE)
                { printData->SetPaperSize(size); }
            }

        gtk_paper_size_free(pageSetupPaperSize);
        }
    printData->SetPaperId(paperId);
    }

//-------------------------------------------------
static void _GtkBeginPrint(GtkPrintOperation* operation,
                           GtkPrintContext* context,
                           _GtkPrintData* printData)
    {
    printData->m_layout = nullptr;
    printData->m_lines = nullptr;
    printData->m_pageLines.clear();
    printData->m_headerAreaHeight = 0;
    printData->m_footerAreaHeight = 0;
    printData->m_layout = gtk_print_context_create_pango_layout(context);

    const gdouble contextWidth{ gtk_print_context_get_width(context) };
    gdouble contextHeight{ gtk_print_context_get_height(context) };

    pango_layout_set_width(printData->m_layout, contextWidth * PANGO_SCALE);
    // measure headers
    if (printData->m_leftPrintHeader.length())
        {
        pango_layout_set_text(printData->m_layout, printData->m_leftPrintHeader, -1);
        pango_layout_get_size(printData->m_layout, nullptr, &printData->m_headerAreaHeight);
        printData->m_headerAreaHeight =
            std::max(40.0, (printData->m_headerAreaHeight / static_cast<gdouble>(PANGO_SCALE)) * 3);
        }
    if (printData->m_centerPrintHeader.length())
        {
        pango_layout_set_text(printData->m_layout, printData->m_centerPrintHeader, -1);
        pango_layout_get_size(printData->m_layout, nullptr, &printData->m_headerAreaHeight);
        printData->m_headerAreaHeight =
            std::max(40.0, (printData->m_headerAreaHeight / static_cast<gdouble>(PANGO_SCALE)) * 3);
        }
    if (printData->m_rightPrintHeader.length())
        {
        pango_layout_set_text(printData->m_layout, printData->m_rightPrintHeader, -1);
        pango_layout_get_size(printData->m_layout, nullptr, &printData->m_headerAreaHeight);
        printData->m_headerAreaHeight =
            std::max(40.0, (printData->m_headerAreaHeight / static_cast<gdouble>(PANGO_SCALE)) * 3);
        }
    // ...and footers
    if (printData->m_leftPrintFooter.length())
        {
        pango_layout_set_text(printData->m_layout, printData->m_leftPrintFooter, -1);
        pango_layout_get_size(printData->m_layout, nullptr, &printData->m_footerAreaHeight);
        printData->m_footerAreaHeight =
            std::max(40.0, (printData->m_footerAreaHeight / static_cast<gdouble>(PANGO_SCALE)) * 3);
        }
    if (printData->m_centerPrintFooter.length())
        {
        pango_layout_set_text(printData->m_layout, printData->m_centerPrintFooter, -1);
        pango_layout_get_size(printData->m_layout, nullptr, &printData->m_footerAreaHeight);
        printData->m_footerAreaHeight =
            std::max(40.0, (printData->m_footerAreaHeight / static_cast<gdouble>(PANGO_SCALE)) * 3);
        }
    if (printData->m_rightPrintFooter.length())
        {
        pango_layout_set_text(printData->m_layout, printData->m_rightPrintFooter, -1);
        pango_layout_get_size(printData->m_layout, nullptr, &printData->m_footerAreaHeight);
        printData->m_footerAreaHeight =
            std::max(40.0, (printData->m_footerAreaHeight / static_cast<gdouble>(PANGO_SCALE)) * 3);
        }
    contextHeight -= printData->m_headerAreaHeight + printData->m_footerAreaHeight;

    // Set the actual text now.
    pango_layout_set_markup(printData->m_layout, printData->m_markupContent.c_str(), -1);

    // Paginate by going through all the lines and measuring them.
    gint layoutHeight{ 0 };
    gint currentPageHeight{ 0 };
    printData->m_lines = pango_layout_get_lines_readonly(printData->m_layout);
    _GtkPageLines currentPageLines{ 0, printData->m_lines, 0 };
    for (auto lines = printData->m_lines;
        lines != nullptr;
        lines = lines->next)
        {
        PangoLayoutLine* line{ static_cast<PangoLayoutLine*>(lines->data) };

        pango_layout_line_get_height(line, &layoutHeight);
        currentPageHeight += layoutHeight / static_cast<gdouble>(PANGO_SCALE);

        // Current line won't fit on this page, so start a new page and put
        // that line at the top of it.
        if (currentPageHeight > contextHeight)
            {
            printData->m_pageLines.push_back(currentPageLines);
            currentPageLines.m_linesStart = lines;
            ++currentPageLines.m_page;
            currentPageLines.m_numberOfLines = 1;
            currentPageHeight = layoutHeight / static_cast<gdouble>(PANGO_SCALE);
            }
        else
            { ++currentPageLines.m_numberOfLines; }
        }
    // add the last straggling page and then set the number of pages for our printout
    printData->m_pageLines.push_back(currentPageLines);

    gtk_print_operation_set_n_pages(operation, static_cast<gint>(printData->m_pageLines.size()));
    }

//-------------------------------------------------
static void _GtkDrawPage([[maybe_unused]] GtkPrintOperation* operation,
                         GtkPrintContext* context,
                         gint pageNr,
                         _GtkPrintData* printData)
    {
    cairo_t* cr = gtk_print_context_get_cairo_context(context);
    const gdouble pageWidth = gtk_print_context_get_width(context);
    const gdouble pageHeight = gtk_print_context_get_height(context);

    const auto expandPrinterString = [pageNr, printData](wxString str)
        {
        str.Replace(L"@PN", std::to_wstring(pageNr + 1), true);
        str.Replace(L"@PC", std::to_wstring(printData->m_pageLines.size()), true);
        return str;
        };

    // Render headers
    auto layout = gtk_print_context_create_pango_layout(context);
    pango_layout_set_width(layout, -1);
    if (printData->m_leftPrintHeader.length())
        {
        pango_layout_set_text(layout, expandPrinterString(printData->m_leftPrintHeader), -1);
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

        cairo_move_to(cr, 0, 0);
        pango_cairo_show_layout(cr, layout);
        }
    if (printData->m_centerPrintHeader.length())
        {
        gint textWidth{ 0 };
        pango_layout_set_text(layout, expandPrinterString(printData->m_centerPrintHeader), -1);
        pango_layout_get_size(layout, &textWidth, nullptr);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

        cairo_move_to(cr, (pageWidth / 2) - ((textWidth / static_cast<gdouble>(PANGO_SCALE)) / 2), 0);
        pango_cairo_show_layout(cr, layout);
        }
    if (printData->m_rightPrintHeader.length())
        {
        gint textWidth{ 0 };
        pango_layout_set_text(layout, expandPrinterString(printData->m_rightPrintHeader), -1);
        pango_layout_get_size(layout, &textWidth, nullptr);
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

        cairo_move_to(cr, pageWidth - (textWidth / static_cast<gdouble>(PANGO_SCALE)), 0);
        pango_cairo_show_layout(cr, layout);
        }
    if (printData->m_leftPrintFooter.length())
        {
        gint textWidth{ 0 }, textHeight{ 0 };
        pango_layout_set_text(layout, expandPrinterString(printData->m_leftPrintFooter), -1);
        pango_layout_get_size(layout, &textWidth, &textHeight);
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

        cairo_move_to(cr, 0, pageHeight - (textHeight / static_cast<gdouble>(PANGO_SCALE)) );
        pango_cairo_show_layout(cr, layout);
        }
    if (printData->m_centerPrintFooter.length())
        {
        gint textWidth{ 0 }, textHeight{ 0 };
        pango_layout_set_text(layout, expandPrinterString(printData->m_centerPrintFooter), -1);
        pango_layout_get_size(layout, &textWidth, &textHeight);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

        cairo_move_to(cr,
                      (pageWidth / 2) - ((textWidth / static_cast<gdouble>(PANGO_SCALE)) / 2),
                      pageHeight - (textHeight / static_cast<gdouble>(PANGO_SCALE)) );
        pango_cairo_show_layout(cr, layout);
        }
    if (printData->m_rightPrintFooter.length())
        {
        gint textWidth{ 0 }, textHeight{ 0 };
        pango_layout_set_text(layout, expandPrinterString(printData->m_rightPrintFooter), -1);
        pango_layout_get_size(layout, &textWidth, &textHeight);
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

        cairo_move_to(cr,
                      pageWidth - (textWidth / static_cast<gdouble>(PANGO_SCALE)),
                      pageHeight - (textHeight / static_cast<gdouble>(PANGO_SCALE)) );
        pango_cairo_show_layout(cr, layout);
        }

    // Render the text on the page, line-by-line.
    cairo_move_to(cr, 0, printData->m_headerAreaHeight);
    auto lines = printData->m_pageLines[pageNr].m_linesStart;
    gint layoutHeight{ 0 };
    for (gint i = 0;
         i < printData->m_pageLines[pageNr].m_numberOfLines;
         ++i, lines = lines->next)
        {
        // Draw the line text.
        PangoLayoutLine* line{ static_cast<PangoLayoutLine*>(lines->data) };
        pango_cairo_show_layout_line(cr, line);
        // Move down to the next line.
        pango_layout_line_get_height(line, &layoutHeight);
        cairo_rel_move_to(cr, 0, (layoutHeight / static_cast<gdouble>(PANGO_SCALE)) );
        }

    g_object_unref(layout);
    }

//-------------------------------------------------
static void _GtkEndPrint(
                  [[maybe_unused]] GtkPrintOperation* operation,
                  [[maybe_unused]] GtkPrintContext* context,
                  _GtkPrintData* printData)
    {
    g_object_unref(printData->m_layout);
    printData->m_layout = nullptr;
    printData->m_lines = nullptr;
    printData->m_pageLines.clear();
    printData->m_headerAreaHeight = 0;
    printData->m_footerAreaHeight = 0;
    }

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
}

/// @returns A GTK text tag into HTML text.
wxString _GtkTextTagToHtmlSpanTag(const GtkTextTag* tag);
/// @returns A GTK text tag into RTF text.
wxString _GtkTextTagToRtfTag(const GtkTextTag* tag,
                             std::vector<wxColour>& colorTable,
                             std::vector<wxString>& fontTable);

/*  Suggestion for bug #59390 on http://bugs.gnome.org,
 *  "load Pango Markup into GtkTextBuffer"
 *
 *  by Tim-Philipp Mueller <t.i.m@zen.co.uk>
 *
 *  Four new functions:
 *  - text_buffer_insert_markup()
 *  - text_buffer_insert_markup_with_tag()
 *  - text_buffer_set_markup()
 *  - text_buffer_set_markup_with_tag()
 * 
 *  Note that as of GTK 3.16 there is a gtk_text_buffer_insert_markup() function,
 *  but its performance is remarkably slower than this patch. I suspect it handles
 *  numerous Pango features that we don't; this is fine, as this function handles
 *  just what is needed for our simple text control's purposes.
 */
void
text_buffer_insert_markup_real (GtkTextBuffer *buffer,
                                GtkTextIter   *textiter,
                                const gchar   *markup,
                                gint           len,
                                GtkTextTag    *extratag);

/**
 * text_buffer_insert_markup:
 * @buffer: a #GtkTextBuffer
 * @iter: a position in the buffer
 * @markup: UTF-8 format text with pango markup to insert
 * @len: length of text in bytes, or -1
 *
 * Inserts @len bytes of @markup at position @iter.  If @len is -1,
 * @text must be null-terminated and will be inserted in its
 * entirety. Emits the "insert_text" signal, possibly multiple
 * times; insertion actually occurs in the default handler for
 * the signal. @iter will point to the end of the inserted text
 * on return
 *
 **/
void
text_buffer_insert_markup (GtkTextBuffer *buffer,
                           GtkTextIter   *iter,
                           const gchar   *markup,
                           gint           len);

/**
 * text_buffer_insert_markup_with_tag:
 * @buffer: a #GtkTextBuffer
 * @iter: a position in the buffer
 * @markup: UTF-8 format text in pango markup format to insert
 * @len: length of text in bytes, or -1
 * @tag: additional text tag to apply to the whole text
 *
 * Just like <literal>text_buffer_insert_markup</literal>, only
 * that an additional tag can be specified that is applied to the
 * whole text to be inserted. This is useful to pass formatting
 * options to the text buffer that cannot be specified with
 * pango markup (e.g. text justification or wrap mode).
 *
 **/
void
text_buffer_insert_markup_with_tag (GtkTextBuffer *buffer,
                                    GtkTextIter   *iter,
                                    const gchar   *markup,
                                    gint           len,
                                    GtkTextTag    *tag);

/**
 * text_buffer_set_markup:
 * @buffer: a #GtkTextBuffer
 * @markup: UTF-8 text with pango markup to insert
 * @len: length of text in bytes
 *
 * Deletes current contents of @buffer, and inserts the text
 * in @markup instead, which may contain pango markup. If
 * @len is -1, @markup must be null-terminated. @markup must be valid UTF-8.
 **/
void
text_buffer_set_markup_with_tag (GtkTextBuffer *buffer,
                                 const gchar   *markup,
                                 gint           len,
                                 GtkTextTag    *tag);

/**
 * text_buffer_set_markup:
 * @buffer: a #GtkTextBuffer
 * @markup: UTF-8 text with pango markup to insert
 * @len: length of text in bytes
 *
 * Deletes current contents of @buffer, and inserts the text
 * in @markup instead, which may contain pango markup. If
 * @len is -1, @markup must be null-terminated. @markup must be valid UTF-8.
 **/
void
text_buffer_set_markup(GtkTextBuffer *buffer,
                       const gchar   *markup,
                       gint           len);

#endif // __WXGTK__
#endif // __GTK_TEXTVIEW_PANGO_MARKUP__
