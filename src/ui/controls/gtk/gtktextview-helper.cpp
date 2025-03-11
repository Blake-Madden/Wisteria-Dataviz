/** @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
        Anthony Bretaudeau (portions of wxWidgets printing code)
    @details This program is free software; you can redistribute it and/or modify
        it under the terms of the 3-Clause BSD License
        (some portions are wxWindows licensed, where noted).

     SPDX-License-Identifier: BSD-3-Clause
     SPDX-License-Identifier: wxWindows
@{*/

#include "gtktextview-helper.h"
#include <algorithm>

// clang-format off
#ifdef __WXGTK__

//-------------------------------------------------
constexpr wxColourBase::ChannelType FloatingPointChannelToByteChannel(const double val)
    { return static_cast<wxColourBase::ChannelType>(std::floor(val >= 1.0 ? 255 : val * 256.0)); }

//-------------------------------------------------
constexpr GdkRGBA PangoAttributeToGdkRGBA(const PangoAttribute* attr)
    {
    return GdkRGBA
        {
        std::clamp(static_cast<double>(((PangoAttrColor*)attr)->color.red) / 65535.0,   0.0, 1.0),
        std::clamp(static_cast<double>(((PangoAttrColor*)attr)->color.green) / 65535.0, 0.0, 1.0),
        std::clamp(static_cast<double>(((PangoAttrColor*)attr)->color.blue) / 65535.0,  0.0, 1.0),
        1.0,
        };
    }

//-------------------------------------------------
wxString _GtkTextTagToHtmlSpanTag(const GtkTextTag* tag)
    {
    wxString text{ L"<span" };
    wxString styleParams{ L" style=\"" };
    // indicators as to whether a tag is set or not
    gboolean bkColorSet, fgColorSet, sizeSet, underlineSet, weightSet, styleSet,
             strikeThroughSet;
    // values to write to
    GdkRGBA* bkColor{ nullptr };
    GdkRGBA* fgColor{ nullptr };
    gdouble size{ 0 };
    gint weight;
    PangoStyle style;
    gboolean underline, strikeThrough;
    gchar* family{ nullptr };
    gchar* font{ nullptr };
    g_object_get(G_OBJECT(tag),
        "background-set", &bkColorSet,
        "foreground-set", &fgColorSet,
        "size-set", &sizeSet,
        "underline-set", &underlineSet,
        "weight-set", &weightSet,
        "style-set", &styleSet,
        "strikethrough-set", &strikeThroughSet,
        "background-rgba", &bkColor,
        "foreground-rgba", &fgColor,
        "font", &font,
        "family", &family,
        "size-points", &size,
        "weight", &weight,
        "style", &style,
        "underline", &underline,
        "strikethrough", &strikeThrough,
        nullptr);
    if (bkColorSet && bkColor)
        {
        styleParams += wxString::Format(L"background-color: rgb(%u, %u, %u);",
            FloatingPointChannelToByteChannel(bkColor->red),
            FloatingPointChannelToByteChannel(bkColor->green),
            FloatingPointChannelToByteChannel(bkColor->blue));
        }
    if (fgColorSet && fgColor)
        {
        styleParams += wxString::Format(L" color: rgb(%u, %u, %u);",
            FloatingPointChannelToByteChannel(fgColor->red),
            FloatingPointChannelToByteChannel(fgColor->green),
            FloatingPointChannelToByteChannel(fgColor->blue));
        }
    if (family != nullptr)
        { styleParams += wxString::Format(L" font-family: %s;", wxString(family, wxConvUTF8)); }
    if (sizeSet && size > 0)
        { styleParams += wxString::Format(L" font-size: %upt;", static_cast<guint>(size)); }
    if (weightSet &&
        (weight == PANGO_WEIGHT_BOLD ||
         weight == PANGO_WEIGHT_ULTRABOLD ||
         weight == PANGO_WEIGHT_HEAVY))
        { styleParams += L" font-weight: bold;"; }
    if (styleSet &&
        (style == PANGO_STYLE_ITALIC ||
         style == PANGO_STYLE_OBLIQUE))
        { styleParams += L" font-style: italic;"; }
    std::vector<wxString> textDecorations;
    if (underlineSet && underline)
        { textDecorations.push_back(L"underline"); }
    if (strikeThroughSet && strikeThrough)
        { textDecorations.push_back(L"line-through"); }
    if (textDecorations.size())
        {
        styleParams += L" text-decoration: ";
        for (const auto& decor : textDecorations)
            { styleParams += decor + L","; }
        // replace final , with a ;
        if (styleParams.length())
            { styleParams[styleParams.length() - 1] = L';'; }
        }
    styleParams += L"\"";

    text += styleParams + L">";

    gdk_rgba_free(bkColor);
    gdk_rgba_free(fgColor);
    g_free(family);
    g_free(font);
    return text;
    }

//-------------------------------------------------
wxString _GtkTextTagToRtfTag(const GtkTextTag* tag,
                            std::vector<wxColour>& colorTable,
                            [[maybe_unused]] std::vector<wxString>& fontTable)
    {
    wxString text{ L" " };
    // indicators as to whether a tag is set or not
    gboolean bkColorSet, fgColorSet, sizeSet, underlineSet, weightSet, styleSet,
        strikeThroughSet;
    // values to write to
    GdkRGBA* bkColor{ nullptr };
    GdkRGBA* fgColor{ nullptr };
    gdouble size{ 0 };
    gint weight;
    PangoStyle style;
    gboolean underline, strikeThrough;
    gchar* family{ nullptr };
    gchar* font{ nullptr };
    g_object_get(G_OBJECT(tag),
        "background-set", &bkColorSet,
        "foreground-set", &fgColorSet,
        "size-set", &sizeSet,
        "underline-set", &underlineSet,
        "weight-set", &weightSet,
        "style-set", &styleSet,
        "strikethrough-set", &strikeThroughSet,
        "background-rgba", &bkColor,
        "foreground-rgba", &fgColor,
        "font", &font,
        "family", &family,
        "size-points", &size,
        "weight", &weight,
        "style", &style,
        "underline", &underline,
        "strikethrough", &strikeThrough,
        nullptr);
    if (bkColorSet && bkColor)
        {
        // search for the color to see if it's already in the color table
        wxColour backgroundColor(FloatingPointChannelToByteChannel(bkColor->red),
            FloatingPointChannelToByteChannel(bkColor->green),
            FloatingPointChannelToByteChannel(bkColor->blue));
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
        wxColour foregroundColor(FloatingPointChannelToByteChannel(fgColor->red),
            FloatingPointChannelToByteChannel(fgColor->green),
            FloatingPointChannelToByteChannel(fgColor->blue));
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
        { text += wxString::Format(L"\\fs%u", static_cast<guint>(size)*2); }
    if (weightSet &&
        (weight == PANGO_WEIGHT_BOLD ||
         weight == PANGO_WEIGHT_ULTRABOLD ||
         weight == PANGO_WEIGHT_HEAVY))
        { text += L"\\b"; }
    if (styleSet &&
        (style == PANGO_STYLE_ITALIC ||
         style == PANGO_STYLE_OBLIQUE))
        { text += L"\\i"; }
    if (underlineSet && underline)
        { text += L"\\ul"; }
    if (strikeThroughSet && strikeThrough)
        { text += L"\\strike"; }

    text += L" ";

    gdk_rgba_free(bkColor);
    gdk_rgba_free(fgColor);
    g_free(family);
    g_free(font);
    return text;
    }

#endif
// clang-format on
