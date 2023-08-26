///////////////////////////////////////////////////////////////////////////////
// Name:        gtktextview-helper.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "gtktextview-helper.h"

#ifdef __WXGTK__

//-------------------------------------------------
void
text_buffer_insert_markup_real (GtkTextBuffer *buffer,
                                GtkTextIter   *textiter,
                                const gchar   *markup,
                                gint           len,
                                GtkTextTag    *extratag)
{
    PangoAttrIterator  *paiter;
    PangoAttrList      *attrlist;
    GtkTextMark        *mark;
    GError             *error = nullptr;
    gchar              *text;

    assert(GTK_IS_TEXT_BUFFER(buffer) && "Invalid text buffer!");
    assert(textiter && "Invalid iterator in text_buffer_insert_markup_real()!");
    assert(markup && "Invalid markup in text_buffer_insert_markup_real()!");
    assert(gtk_text_iter_get_buffer(textiter) == buffer &&
        "Iterator is not pointing to the correct buffer in text_buffer_insert_markup_real()!");

    g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (textiter != NULL);
    g_return_if_fail (markup != NULL);
    g_return_if_fail (gtk_text_iter_get_buffer (textiter) == buffer);

    if (len == 0)
        return;
    len = (len == -1) ? strlen(markup) : len;

    if (!pango_parse_markup(markup, len, 0, &attrlist, &text, NULL, &error))
    {
        g_warning("Invalid markup string: %s", error->message);
        wxLogError("Invalid markup string: %s", error->message);
        g_error_free(error);
        return;
    }

    if (attrlist == NULL)
    {
        gtk_text_buffer_insert(buffer, textiter, text, len);
        return;
    }

    /* create mark with right gravity */
    mark = gtk_text_buffer_create_mark(buffer, NULL, textiter, FALSE);

    paiter = pango_attr_list_get_iterator(attrlist);

    do
    {
        PangoAttribute *attr;
        GtkTextTag     *tag;
        gint            start, end;

        pango_attr_iterator_range(paiter, &start, &end);

        if (end == G_MAXINT)  /* last chunk */
            end = strlen(text);

        tag = gtk_text_tag_new(NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_LANGUAGE)))
            g_object_set(tag, "language", pango_language_to_string(((PangoAttrLanguage*)attr)->value), NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FAMILY)))
            g_object_set(tag, "family", ((PangoAttrString*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STYLE)))
            g_object_set(tag, "style", ((PangoAttrInt*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_WEIGHT)))
            g_object_set(tag, "weight", ((PangoAttrInt*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_VARIANT)))
            g_object_set(tag, "variant", ((PangoAttrInt*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRETCH)))
            g_object_set(tag, "stretch", ((PangoAttrInt*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SIZE)))
            g_object_set(tag, "size", ((PangoAttrInt*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FONT_DESC)))
            g_object_set(tag, "font-desc", ((PangoAttrFontDesc*)attr)->desc, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FOREGROUND)))
        {
            GdkColor col = { 0,
                             ((PangoAttrColor*)attr)->color.red,
                             ((PangoAttrColor*)attr)->color.green,
                             ((PangoAttrColor*)attr)->color.blue
                           };

            g_object_set(tag, "foreground-gdk", &col, NULL);
        }

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_BACKGROUND)))
        {
            GdkColor col = { 0,
                             ((PangoAttrColor*)attr)->color.red,
                             ((PangoAttrColor*)attr)->color.green,
                             ((PangoAttrColor*)attr)->color.blue
                           };

            g_object_set(tag, "background-gdk", &col, NULL);
        }

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_UNDERLINE)))
            g_object_set(tag, "underline", ((PangoAttrInt*)attr)->value, NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRIKETHROUGH)))
            g_object_set(tag, "strikethrough", (gboolean)(((PangoAttrInt*)attr)->value != 0), NULL);

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_RISE)))
            g_object_set(tag, "rise", ((PangoAttrInt*)attr)->value, NULL);

        /* PANGO_ATTR_SHAPE cannot be defined via markup text */

        if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SCALE)))
            g_object_set(tag, "scale", ((PangoAttrFloat*)attr)->value, NULL);

        gtk_text_tag_table_add(gtk_text_buffer_get_tag_table(buffer), tag);

        if (extratag)
        {
            gtk_text_buffer_insert_with_tags(buffer, textiter, text+start, end - start, tag, extratag, NULL);
        }
        else
        {
            gtk_text_buffer_insert_with_tags(buffer, textiter, text+start, end - start, tag, NULL);
        }
        g_object_unref(G_OBJECT(tag));

        /* mark had right gravity, so it should be
         *  at the end of the inserted text now */
        gtk_text_buffer_get_iter_at_mark(buffer, textiter, mark);
    }
    while (pango_attr_iterator_next(paiter));

    gtk_text_buffer_delete_mark(buffer, mark);
}

//-------------------------------------------------
void
text_buffer_insert_markup (GtkTextBuffer *buffer,
                           GtkTextIter   *iter,
                           const gchar   *markup,
                           gint           len)
{
    text_buffer_insert_markup_real (buffer, iter, markup, len, NULL);
}

//-------------------------------------------------
void
text_buffer_insert_markup_with_tag (GtkTextBuffer *buffer,
                                    GtkTextIter   *iter,
                                    const gchar   *markup,
                                    gint           len,
                                    GtkTextTag    *tag)
{
    text_buffer_insert_markup_real (buffer, iter, markup, len, tag);
}

//-------------------------------------------------
void
text_buffer_set_markup_with_tag (GtkTextBuffer *buffer,
                                 const gchar   *markup,
                                 gint           len,
                                 GtkTextTag    *tag)
{
    assert(GTK_IS_TEXT_BUFFER(buffer) && "Invalid text buffer!");
    assert(markup && "Null markup passed to text_buffer_set_markup_with_tag()!");

    g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (markup != nullptr);

    if (len < 0)
        len = strlen (markup);

    // clear current content
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    gtk_text_buffer_delete (buffer, &start, &end);

    if (len > 0)
    {
        gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
        text_buffer_insert_markup_with_tag (buffer, &start, markup, len, tag);
    }
}

//-------------------------------------------------
void
text_buffer_set_markup (GtkTextBuffer *buffer,
                        const gchar   *markup,
                        gint           len)
{
    text_buffer_set_markup_with_tag(buffer, markup, len, NULL);
}

//-------------------------------------------------
wxString wxGtkTextTagToHtmlSpanTag(const GtkTextTag* tag)
    {
    wxString text = L"<span";
    wxString styleParams = L" style=\"";
    // indicators as to whether a tag is set or not
    gboolean bkColorSet, fgColorSet, sizeSet, underlineSet, weigthSet, styleSet;
    // values to write to
    GdkRGBA* bkColor = nullptr;
    GdkRGBA* fgColor = nullptr;
    gdouble size = 0;
    gboolean weigth, style, underline;
    gchar* family = NULL;
    gchar* font = NULL;
    g_object_get(G_OBJECT(tag),
        "background-set", &bkColorSet,
        "foreground-set", &fgColorSet,
        "size-set", &sizeSet,
        "underline-set", &underlineSet,
        "weight-set", &weigthSet,
        "style-set", &styleSet,
        "background-gdk", &bkColor,
        "foreground-gdk", &fgColor,
        "font", &font,
        "family", &family,
        "size-points", &size,
        "weight", &weigth,
        "style", &style,
        "underline", &underline,
        NULL);
    if (bkColorSet && bkColor)
        {
        styleParams += wxString::Format(L"background-color: rgb(%u, %u, %u);",
            UintToByte(bkColor->red), UintToByte(bkColor->green), UintToByte(bkColor->blue));
        }
    if (fgColorSet && fgColor)
        {
        styleParams += wxString::Format(L" color: rgb(%u, %u, %u);",
            UintToByte(fgColor->red), UintToByte(fgColor->green), UintToByte(fgColor->blue));
        }
    styleParams += wxString::Format(L" font-family: %s;", wxString(family, *wxConvCurrent));
    if (sizeSet && size > 0)
        { styleParams += wxString::Format(L" font-size: %upt;", static_cast<guint>(size)); }
    if (weigthSet &&
        (weigth == PANGO_WEIGHT_BOLD ||
         weigth == PANGO_WEIGHT_ULTRABOLD ||
         weigth == PANGO_WEIGHT_HEAVY))
        { styleParams += L" font-weight: bold;"; }
    if (styleSet &&
        (style == PANGO_STYLE_ITALIC ||
         style == PANGO_STYLE_OBLIQUE))
        { styleParams += L" font-style: italic;"; }
    if (underlineSet && underline)
        { styleParams += L" text-decoration: underline;"; }
    styleParams += L"\"";

    text += styleParams + L">";

    gdk_rgba_free(bkColor);
    gdk_rgba_free(fgColor);
    g_free(family);
    g_free(font);
    return text;
    }

//-------------------------------------------------
wxString wxGtkTextTagToRtfTag(const GtkTextTag* tag,
                              std::vector<wxColour>& colorTable,
                              std::vector<wxString>& fontTable)
    {
    wxString text = L" ";
    // indicators as to whether a tag is set or not
    gboolean bkColorSet, fgColorSet, sizeSet, underlineSet, weigthSet, styleSet;
    // values to write to
    GdkRGBA* bkColor = nullptr;
    GdkRGBA* fgColor = nullptr;
    gdouble size = 0;
    gboolean weigth, style, underline;
    gchar* family = nullptr;
    gchar* font = nullptr;
    g_object_get(G_OBJECT(tag),
        "background-set", &bkColorSet,
        "foreground-set", &fgColorSet,
        "size-set", &sizeSet,
        "underline-set", &underlineSet,
        "weight-set", &weigthSet,
        "style-set", &styleSet,
        "background-gdk", &bkColor,
        "foreground-gdk", &fgColor,
        "font", &font,
        "family", &family,
        "size-points", &size,
        "weight", &weigth,
        "style", &style,
        "underline", &underline,
        NULL);
    if (bkColorSet && bkColor)
        {
        // search for the color to see if it's already in the color table
        wxColour backgroundColor(UintToByte(bkColor->red), UintToByte(bkColor->green), UintToByte(bkColor->blue));
        auto colorPos = std::find(colorTable.cbegin(), colorTable.cend(), backgroundColor);
        if (colorPos == colorTable.cend())
            {
            colorTable.push_back(backgroundColor);
            text += wxString::Format(L"\\highlight%zu", colorTable.size());
            }
        else
            {
            text += wxString::Format(L"\\highlight%zu", (colorPos - colorTable.cbegin()) + 1);
            }
        }
    if (fgColorSet && fgColor)
        {
        // search for the color to see if it's already in the color table
        wxColour foregroundColor(UintToByte(fgColor->red), UintToByte(fgColor->green), UintToByte(fgColor->blue));
        auto colorPos = std::find(colorTable.cbegin(), colorTable.cend(), foregroundColor);
        if (colorPos == colorTable.cend())
            {
            colorTable.push_back(foregroundColor);
            text += wxString::Format(L"\\cf%zu", colorTable.size());
            }
        else
            {
            text += wxString::Format(L"\\cf%zu", (colorPos - colorTable.cbegin()) + 1);
            }
        }
    if (sizeSet && size > 0)
        { text += wxString::Format(L"\\fs%zu", static_cast<guint>(size)*2); }
    if (weigthSet &&
        (weigth == PANGO_WEIGHT_BOLD ||
         weigth == PANGO_WEIGHT_ULTRABOLD ||
         weigth == PANGO_WEIGHT_HEAVY))
        { text += L"\\b"; }
    if (styleSet &&
        (style == PANGO_STYLE_ITALIC ||
         style == PANGO_STYLE_OBLIQUE))
        { text += L"\\i"; }
    if (underlineSet && underline)
        { text += L"\\ul"; }

    text += L" ";

    gdk_rgba_free(bkColor);
    gdk_rgba_free(fgColor);
    g_free(family);
    g_free(font);
    return text;
    }

#endif
