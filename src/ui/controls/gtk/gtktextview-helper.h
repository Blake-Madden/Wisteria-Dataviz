/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __GTK_TEXTVIEW_PANGO_MARKUP__
#define __GTK_TEXTVIEW_PANGO_MARKUP__

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gtk/gtktextiter.h>
#include <gtk/gtktexttag.h>
#include <gtk/gtktextchild.h>
#include <gtk/gtktextiter.h>
#include <string>
#include <vector>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/gtk/print.h>

/// @todo EXPERIMENTAL and INCOMPLETE!
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
//-------------------------------------------------
static void _GtkBeginPrint(GtkPrintOperation* operation,
                           GtkPrintContext* context,
                           _GtkPrintData* printData)
    {
    printData->m_layout = nullptr;
    printData->m_lines = nullptr;
    printData->m_pageLines.clear();
    printData->m_headerAreaHeight = 0;
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
    }
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
}

/// END EXPERIMENTAL

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
