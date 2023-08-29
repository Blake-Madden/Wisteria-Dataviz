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

inline guint8 UintToByte(const guint16 value)
    { return (value == 0) ? 0 : value/256u; }

/// @returns A GTK text tag into HTML text.
wxString wxGtkTextTagToHtmlSpanTag(const GtkTextTag* tag);
/// @returns A GTK text tag into RTF text.
wxString wxGtkTextTagToRtfTag(const GtkTextTag* tag,
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
 *  numberous Pango features that we don't; this is fine, as this function handles
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
