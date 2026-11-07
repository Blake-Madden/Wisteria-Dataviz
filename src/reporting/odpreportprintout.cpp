///////////////////////////////////////////////////////////////////////////////
// Name:        odpreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "odpreportprintout.h"
#include "../base/colorschemenames.h"
#include "../math/safe_math.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <variant>
#include <wx/arrstr.h>
#include <wx/datetime.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

//------------------------------------------------------
double Wisteria::OdpExportOptions::GetSlideWidthInches() const
    {
    switch (m_slideSize)
        {
    case SlideSize::Standard4x3:
        return SLIDE_WIDTH_4X3_INCHES;
    case SlideSize::Custom:
        return std::clamp(m_customWidthInches, 1.0, MAX_SLIDE_INCHES);
    case SlideSize::Widescreen16x9:
    default:
        return SLIDE_WIDTH_16X9_INCHES;
        }
    }

//------------------------------------------------------
double Wisteria::OdpExportOptions::GetSlideHeightInches() const
    {
    return (m_slideSize == SlideSize::Custom) ?
               std::clamp(m_customHeightInches, 1.0, MAX_SLIDE_INCHES) :
               SLIDE_HEIGHT_INCHES;
    }

//------------------------------------------------------
wxSize Wisteria::OdpExportOptions::GetSlideSizeDIPs() const
    {
    return { static_cast<int>(std::llround(GetSlideWidthInches() * 96)),
             static_cast<int>(std::llround(GetSlideHeightInches() * 96)) };
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::ToInches(const double value)
    {
    return wxString::FromCDouble(value, 4) + L"in";
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::ColorToOdfHex(const wxColour& color)
    {
    return L"#" + ColorToHex(color);
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildManifestXml(const std::vector<wxString>& pictureFileNames)
    {
    wxString xml{
        L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        L"<manifest:manifest "
        L"xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" "
        L"manifest:version=\"1.3\">"
        L"<manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.3\" "
        L"manifest:media-type=\"application/vnd.oasis.opendocument.presentation\"/>"
        L"<manifest:file-entry manifest:full-path=\"content.xml\" manifest:media-type=\"text/"
        L"xml\"/>"
        L"<manifest:file-entry manifest:full-path=\"styles.xml\" manifest:media-type=\"text/"
        L"xml\"/>"
        L"<manifest:file-entry manifest:full-path=\"meta.xml\" manifest:media-type=\"text/xml\"/>"
    };
    for (const auto& pictureFileName : pictureFileNames)
        {
        xml += wxString::Format(L"<manifest:file-entry manifest:full-path=\"Pictures/%s\" "
                                L"manifest:media-type=\"image/svg+xml\"/>",
                                pictureFileName);
        }
    xml += L"</manifest:manifest>";
    return xml;
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildPicturePlacementAttributes(const double slideWidthInches,
                                                                    const double slideHeightInches,
                                                                    const int imageWidth,
                                                                    const int imageHeight)
    {
    double offsetX{ 0.0 };
    double offsetY{ 0.0 };
    double extentWidth{ slideWidthInches };
    double extentHeight{ slideHeightInches };

    if (imageWidth > 0 && imageHeight > 0)
        {
        const double slideAspect{ safe_divide(slideWidthInches, slideHeightInches) };
        const double imageAspect{ safe_divide(static_cast<double>(imageWidth),
                                              static_cast<double>(imageHeight)) };
        if (imageAspect > slideAspect)
            {
            extentWidth = slideWidthInches;
            extentHeight =
                safe_divide(slideWidthInches * imageHeight, static_cast<double>(imageWidth));
            offsetY = (slideHeightInches - extentHeight) / 2;
            }
        else
            {
            extentHeight = slideHeightInches;
            extentWidth =
                safe_divide(slideHeightInches * imageWidth, static_cast<double>(imageHeight));
            offsetX = (slideWidthInches - extentWidth) / 2;
            }
        }

    return wxString::Format(L"svg:x=\"%s\" svg:y=\"%s\" svg:width=\"%s\" svg:height=\"%s\"",
                            ToInches(offsetX), ToInches(offsetY), ToInches(extentWidth),
                            ToInches(extentHeight));
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildNotesXml(const wxString& notes)
    {
    if (notes.empty())
        {
        return {};
        }

    wxString paragraphs;
    // notes text is free-form, so don't treat '\' as an escape character
    const wxArrayString noteLines = wxSplit(notes, L'\n', L'\0');
    for (const auto& noteLine : noteLines)
        {
        if (noteLine.empty())
            {
            paragraphs += L"<text:p/>";
            }
        else
            {
            paragraphs += wxString::Format(L"<text:p>%s</text:p>", EscapeXml(noteLine));
            }
        }

    return wxString::Format(
        L"<presentation:notes>"
        L"<draw:frame draw:layer=\"layout\" svg:x=\"0.5in\" svg:y=\"0.5in\" svg:width=\"7in\" "
        L"svg:height=\"9in\"><draw:text-box>%s</draw:text-box></draw:frame>"
        L"</presentation:notes>",
        paragraphs);
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildTransitionAttributes(const OdpExportOptions& options)
    {
    using Transition = OdpExportOptions::Transition;
    using Speed = OdpExportOptions::TransitionSpeed;

    if (options.m_transition == Transition::None && !options.m_advanceAutomatically)
        {
        return {};
        }

    // odf:transition-style's enumeration has no "push", "wipe", "split", or "cut" value
    // (it predates those effects), so these map to the closest available analog.
    // Morph and Cut have no ODF/Impress equivalent at all; Cut (an instant change) maps
    // to "none" and Morph falls back to a dissolve.
    wxString styleValue;
    switch (options.m_transition)
        {
    case Transition::None:
    case Transition::Cut:
        styleValue = L"none";
        break;
    case Transition::Fade:
        styleValue = L"fade-from-center";
        break;
    case Transition::Morph:
        styleValue = L"dissolve";
        break;
    case Transition::Push:
        styleValue = L"move-from-right";
        break;
    case Transition::Wipe:
        styleValue = L"uncover-to-left";
        break;
    case Transition::Split:
        styleValue = L"open-horizontal";
        break;
        }

    wxString speed{ L"medium" };
    if (options.m_transitionSpeed == Speed::Slow)
        {
        speed = L"slow";
        }
    else if (options.m_transitionSpeed == Speed::Fast)
        {
        speed = L"fast";
        }

    wxString attribs;
    if (options.m_transition != Transition::None)
        {
        attribs += wxString::Format(
            L" presentation:transition-style=\"%s\" presentation:transition-speed=\"%s\"",
            styleValue, speed);
        }
    if (options.m_advanceAutomatically)
        {
        attribs += wxString::Format(
            L" presentation:transition-type=\"automatic\" presentation:duration=\"PT%dS\"",
            std::max(0, options.m_advanceSeconds));
        }
    else
        {
        attribs += L" presentation:transition-type=\"manual\"";
        }
    return attribs;
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildTitleSlideXml(const OdpExportOptions& options,
                                                       const double slideWidthInches,
                                                       const double slideHeightInches,
                                                       const wxString& transitionAttributes,
                                                       wxString& automaticStyles)
    {
    const auto colorScheme{ Colors::Schemes::ColorSchemeCatalog::FromKey(
        options.m_titleSlideTheme) };
    const bool isThemed{ colorScheme != nullptr };

    const wxColour bgColor1{ isThemed ? colorScheme->GetColor(0) : *wxWHITE };
    const wxColour bgColor2{ isThemed ? colorScheme->GetColor(1) : *wxWHITE };
    const wxColour accentColor{ isThemed ? colorScheme->GetColor(2) : *wxBLACK };
    const wxColour blendedBg{ static_cast<unsigned char>((bgColor1.Red() + bgColor2.Red()) / 2),
                              static_cast<unsigned char>((bgColor1.Green() + bgColor2.Green()) / 2),
                              static_cast<unsigned char>((bgColor1.Blue() + bgColor2.Blue()) / 2) };
    const wxColour titleColor{ !isThemed ? *wxBLACK :
                               Colors::ColorContrast::IsDark(blendedBg) ?
                                           *wxWHITE :
                                           wxColour{ 0x20, 0x20, 0x20 } };

    const wxString pageStyleName{ L"dpTitle" };
    const wxString gradientName{ L"titleGradient" };
    const wxString titleParaStyle{ L"tsTitle" };
    const wxString subtitleParaStyle{ L"tsSubtitle" };
    const wxString publisherParaStyle{ L"tsPublisher" };
    const wxString titleFrameStyle{ L"tsTitleFrame" };
    const wxString subtitleFrameStyle{ L"tsSubtitleFrame" };
    const wxString publisherFrameStyle{ L"tsPublisherFrame" };
    const wxString ruleStyle{ L"tsRule" };
    const wxString circleStyle{ L"tsCircle" };
    const wxString barStyle{ L"tsBar" };

    // borderless/fill-less frame styles for the text boxes below, vertically anchored
    // to match the pptx title slide's anchor="ctr"/"t"/"b" body placeholders
    automaticStyles +=
        wxString::Format(L"<style:style style:name=\"%s\" style:family=\"graphic\">"
                         L"<style:graphic-properties draw:fill=\"none\" draw:stroke=\"none\" "
                         L"draw:textarea-vertical-align=\"middle\"/></style:style>"
                         L"<style:style style:name=\"%s\" style:family=\"graphic\">"
                         L"<style:graphic-properties draw:fill=\"none\" draw:stroke=\"none\" "
                         L"draw:textarea-vertical-align=\"top\"/></style:style>"
                         L"<style:style style:name=\"%s\" style:family=\"graphic\">"
                         L"<style:graphic-properties draw:fill=\"none\" draw:stroke=\"none\" "
                         L"draw:textarea-vertical-align=\"bottom\"/></style:style>",
                         titleFrameStyle, subtitleFrameStyle, publisherFrameStyle);

    // The drawing-page background is a gradient when themed, and plain white otherwise.
    // Transition/auto-advance are also drawing-page properties, not draw:page attributes.
    if (isThemed)
        {
        automaticStyles += wxString::Format(
            L"<draw:gradient draw:name=\"%s\" draw:style=\"linear\" draw:start-color=\"%s\" "
            L"draw:end-color=\"%s\" draw:angle=\"450\"/>"
            L"<style:style style:name=\"%s\" style:family=\"drawing-page\">"
            L"<style:drawing-page-properties draw:fill=\"gradient\" "
            L"draw:fill-gradient-name=\"%s\"%s/></style:style>",
            gradientName, ColorToOdfHex(bgColor1), ColorToOdfHex(bgColor2), pageStyleName,
            gradientName, transitionAttributes);
        }
    else
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"%s\" style:family=\"drawing-page\">"
            L"<style:drawing-page-properties draw:fill=\"solid\" draw:fill-color=\"#FFFFFF\"%s/>"
            L"</style:style>",
            pageStyleName, transitionAttributes);
        }

    wxArrayString subtitleLines;
    if (!options.m_author.empty())
        {
        subtitleLines.Add(options.m_author);
        }
    const bool hasSubtitle{ !subtitleLines.empty() };

    const double marginX{ slideWidthInches * (isThemed ? 0.12 : 0.08) };
    const double boxWidth{ slideWidthInches - (marginX * 2) };
    const double titleY{ slideHeightInches * (hasSubtitle ? 0.38 : 0.42) };
    const double titleHeight{ slideHeightInches * 0.18 };

    automaticStyles += wxString::Format(
        L"<style:style style:name=\"%s\" style:family=\"paragraph\">"
        L"<style:paragraph-properties fo:text-align=\"center\"/>"
        L"<style:text-properties fo:font-size=\"44pt\" fo:font-weight=\"bold\"%s/>"
        L"</style:style>",
        titleParaStyle,
        isThemed ? wxString::Format(L" fo:color=\"%s\"", ColorToOdfHex(titleColor)) : wxString{});

    wxString titleBox{ wxString::Format(
        L"<draw:frame draw:style-name=\"%s\" draw:layer=\"layout\" "
        L"svg:x=\"%s\" svg:y=\"%s\" svg:width=\"%s\" svg:height=\"%s\">"
        L"<draw:text-box><text:p text:style-name=\"%s\">%s</text:p></draw:text-box></draw:frame>",
        titleFrameStyle, ToInches(marginX), ToInches(titleY), ToInches(boxWidth),
        ToInches(titleHeight), titleParaStyle, EscapeXml(options.m_title)) };

    wxString subtitleBox;
    if (hasSubtitle)
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"%s\" style:family=\"paragraph\">"
            L"<style:paragraph-properties fo:text-align=\"center\"/>"
            L"<style:text-properties fo:font-size=\"20pt\"%s/></style:style>",
            subtitleParaStyle,
            isThemed ? wxString::Format(L" fo:color=\"%s\"", ColorToOdfHex(titleColor)) :
                       wxString{});

        wxString subtitleParas;
        for (const auto& subtitleLine : subtitleLines)
            {
            subtitleParas += wxString::Format(L"<text:p text:style-name=\"%s\">%s</text:p>",
                                              subtitleParaStyle, EscapeXml(subtitleLine));
            }
        const double subtitleY{ slideHeightInches * (isThemed ? 0.64 : 0.60) };
        const double subtitleHeight{ slideHeightInches * 0.22 };
        subtitleBox = wxString::Format(
            L"<draw:frame draw:style-name=\"%s\" draw:layer=\"layout\" svg:x=\"%s\" svg:y=\"%s\" "
            L"svg:width=\"%s\" svg:height=\"%s\"><draw:text-box>%s</draw:text-box></draw:frame>",
            subtitleFrameStyle, ToInches(marginX), ToInches(subtitleY), ToInches(boxWidth),
            ToInches(subtitleHeight), subtitleParas);
        }

    // thin accent rule centered beneath the title, only for themed slides
    wxString titleRuleXml;
    if (isThemed)
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"%s\" style:family=\"graphic\">"
            L"<style:graphic-properties draw:fill=\"solid\" draw:fill-color=\"%s\" "
            L"draw:stroke=\"none\"/></style:style>",
            ruleStyle, ColorToOdfHex(accentColor));

        const double ruleWidth{ slideWidthInches * 0.10 };
        const double ruleHeight{ slideHeightInches * 0.006 };
        const double ruleX{ (slideWidthInches - ruleWidth) / 2 };
        const double ruleY{ titleY + titleHeight + (slideHeightInches * 0.015) };
        titleRuleXml = wxString::Format(
            L"<draw:frame draw:style-name=\"%s\" draw:layer=\"layout\" svg:x=\"%s\" svg:y=\"%s\" "
            L"svg:width=\"%s\" svg:height=\"%s\"><draw:rect/></draw:frame>",
            ruleStyle, ToInches(ruleX), ToInches(ruleY), ToInches(ruleWidth), ToInches(ruleHeight));
        }

    wxString publisherBox;
    if (!options.m_publisher.empty())
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"%s\" style:family=\"paragraph\">"
            L"<style:paragraph-properties fo:text-align=\"center\"/>"
            L"<style:text-properties fo:font-size=\"12pt\"%s/></style:style>",
            publisherParaStyle,
            isThemed ? wxString::Format(L" fo:color=\"%s\"", ColorToOdfHex(titleColor)) :
                       wxString{});

        const double publisherY{ slideHeightInches * 0.92 };
        const double publisherHeight{ slideHeightInches * 0.06 };
        publisherBox = wxString::Format(
            L"<draw:frame draw:style-name=\"%s\" draw:layer=\"layout\" svg:x=\"%s\" svg:y=\"%s\" "
            L"svg:width=\"%s\" svg:height=\"%s\"><draw:text-box>"
            L"<text:p text:style-name=\"%s\">%s</text:p></draw:text-box></draw:frame>",
            publisherFrameStyle, ToInches(marginX), ToInches(publisherY), ToInches(boxWidth),
            ToInches(publisherHeight), publisherParaStyle, EscapeXml(options.m_publisher));
        }

    // large, subtle accent circle bleeding off the top-right corner
    wxString accentCircleXml;
    if (isThemed)
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"%s\" style:family=\"graphic\">"
            L"<style:graphic-properties draw:fill=\"solid\" draw:fill-color=\"%s\" "
            L"draw:opacity=\"16%%\" draw:stroke=\"none\"/></style:style>",
            circleStyle, ColorToOdfHex(accentColor));

        const double circleSize{ slideHeightInches * 0.95 };
        const double circleX{ slideWidthInches * 0.60 };
        const double circleY{ -(slideHeightInches * 0.40) };
        accentCircleXml = wxString::Format(
            L"<draw:frame draw:style-name=\"%s\" draw:layer=\"layout\" svg:x=\"%s\" svg:y=\"%s\" "
            L"svg:width=\"%s\" svg:height=\"%s\"><draw:ellipse/></draw:frame>",
            circleStyle, ToInches(circleX), ToInches(circleY), ToInches(circleSize),
            ToInches(circleSize));
        }

    // solid accent bar running the full height of the left edge
    wxString accentBarXml;
    if (isThemed)
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"%s\" style:family=\"graphic\">"
            L"<style:graphic-properties draw:fill=\"solid\" draw:fill-color=\"%s\" "
            L"draw:stroke=\"none\"/></style:style>",
            barStyle, ColorToOdfHex(accentColor));

        const double barWidth{ slideWidthInches * 0.016 };
        accentBarXml = wxString::Format(
            L"<draw:frame draw:style-name=\"%s\" draw:layer=\"layout\" svg:x=\"0in\" "
            L"svg:y=\"0in\" svg:width=\"%s\" svg:height=\"%s\"><draw:rect/></draw:frame>",
            barStyle, ToInches(barWidth), ToInches(slideHeightInches));
        }

    return wxString::Format(L"<draw:page draw:name=\"Title\" draw:style-name=\"%s\" "
                            L"draw:master-page-name=\"Default\">"
                            L"%s%s%s%s%s%s</draw:page>",
                            pageStyleName, accentCircleXml, accentBarXml, titleBox, titleRuleXml,
                            subtitleBox, publisherBox);
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildTableCellStyles(const double cellMarginX,
                                                         const size_t slideNumber)
    {
    const wxString marginX{ ToInches(cellMarginX) };
    const wxString marginY{ ToInches(TABLE_CELL_MARGIN_Y_INCHES) };
    const wxString paddingAttributes{ wxString::Format(
        L"fo:padding-left=\"%s\" fo:padding-right=\"%s\" fo:padding-top=\"%s\" "
        L"fo:padding-bottom=\"%s\"",
        marginX, marginX, marginY, marginY) };

    const auto buildCellStyle =
        [&](const wchar_t* const cellType, const std::wstring_view fillColor)
    {
        const wxString fillAttributes{
            fillColor.empty() ? wxString{ L"draw:fill=\"none\"" } :
                                wxString::Format(L"draw:fill=\"solid\" draw:fill-color=\"#%s\"",
                                                 wxString{ fillColor })
        };
        const wxString backgroundAttribute{ fillColor.empty() ?
                                                wxString{} :
                                                wxString::Format(L" fo:background-color=\"#%s\"",
                                                                 wxString{ fillColor }) };
        // Impress reads cell fills from the graphic properties
        return wxString::Format(
            L"<style:style style:name=\"tc%zu%s\" style:family=\"table-cell\">"
            L"<style:graphic-properties %s draw:textarea-vertical-align=\"middle\" %s/>"
            L"<style:table-cell-properties style:vertical-align=\"middle\" "
            L"fo:border-bottom=\"0.5pt solid #%s\"%s %s/></style:style>",
            slideNumber, cellType, fillAttributes, paddingAttributes, wxString{ TABLE_LINE_COLOR },
            backgroundAttribute, paddingAttributes);
    };

    return buildCellStyle(L"Header", TABLE_HEADER_COLOR) +
           buildCellStyle(L"Body", std::wstring_view{}) + buildCellStyle(L"Band", TABLE_BAND_COLOR);
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildTableStyles()
    {
    return wxString::Format(
        L"<style:style style:name=\"tsTableTitle\" style:family=\"paragraph\">"
        L"<style:text-properties fo:font-size=\"%dpt\" fo:font-weight=\"bold\"/>"
        L"</style:style>",
        TABLE_TITLE_FONT_POINTS);
    }

//------------------------------------------------------
wxString Wisteria::ReportOdpExport::BuildTableSlideXml(const TableSlide& slide,
                                                       const size_t slideNumber,
                                                       wxString& automaticStyles)
    {
    wxString frames;
    double tableWidth{ 0.0 };
    wxString columnsXml;
    for (size_t col = 0; col < slide.m_columnWidths.size(); ++col)
        {
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"tco%zu_%zu\" style:family=\"table-column\">"
            L"<style:table-column-properties style:column-width=\"%s\"/></style:style>",
            slideNumber, col, ToInches(slide.m_columnWidths[col]));
        columnsXml += wxString::Format(L"<table:table-column table:style-name=\"tco%zu_%zu\"/>",
                                       slideNumber, col);
        tableWidth += slide.m_columnWidths[col];
        }

    const auto addRowStyle = [&](const wxString& styleName, const double rowHeight)
    {
        automaticStyles +=
            wxString::Format(L"<style:style style:name=\"%s\" style:family=\"table-row\">"
                             L"<style:table-row-properties style:row-height=\"%s\"/></style:style>",
                             styleName, ToInches(rowHeight));
    };

    automaticStyles += BuildTableCellStyles(slide.m_cellMarginX, slideNumber);

    // paragraph styles for the header (h) and body (b) in each alignment
    for (const wchar_t* const rowType : { L"h", L"b" })
        {
        const bool isHeader{ rowType[0] == L'h' };
        for (const wchar_t* const textAlignment : { L"left", L"center", L"right" })
            {
            automaticStyles += wxString::Format(
                L"<style:style style:name=\"tp%zu%s%s\" style:family=\"paragraph\">"
                L"<style:paragraph-properties fo:text-align=\"%s\"/>"
                L"<style:text-properties fo:font-size=\"%dpt\" fo:color=\"#000000\"%s/>"
                L"</style:style>",
                slideNumber, rowType, textAlignment, textAlignment, slide.m_fontPoints,
                isHeader ? L" fo:font-weight=\"bold\"" : L"");
            }
        }

    const auto buildCell = [slideNumber](const wxString& text, const wxAlignment alignment,
                                         const wchar_t* const rowType,
                                         const wchar_t* const cellType) -> wxString
    {
        const wxString textAlignment{ (alignment == wxALIGN_RIGHT)  ? L"right" :
                                      (alignment == wxALIGN_CENTRE) ? L"center" :
                                                                      L"left" };
        return wxString::Format(L"<table:table-cell table:style-name=\"tc%zu%s\">"
                                L"<text:p text:style-name=\"tp%zu%s%s\">%s</text:p>"
                                L"</table:table-cell>",
                                slideNumber, cellType, slideNumber, rowType, textAlignment,
                                EscapeXmlAttribute(text));
    };

    const wxString headerRowStyle{ wxString::Format(L"tro%zu", slideNumber) };
    addRowStyle(headerRowStyle, slide.m_headerHeight);
    wxString rowsXml{ wxString::Format(L"<table:table-row table:style-name=\"%s\">",
                                       headerRowStyle) };
    for (const auto& column : slide.m_columns)
        {
        rowsXml += buildCell(column.m_label, column.m_alignment, L"h", L"Header");
        }
    rowsXml += L"</table:table-row>";
    for (size_t row = 0; row < slide.m_rows.size(); ++row)
        {
        const TableSlideRow& slideRow{ slide.m_rows[row] };
        const wxString rowStyle{ wxString::Format(L"tro%zu_%zu", slideNumber, row) };
        addRowStyle(rowStyle, slideRow.m_height);
        rowsXml += wxString::Format(L"<table:table-row table:style-name=\"%s\">", rowStyle);
        for (size_t col = 0; col < slide.m_columns.size(); ++col)
            {
            rowsXml += buildCell(slideRow.m_cells[col], slideRow.m_alignments[col], L"b",
                                 slideRow.m_isBanded ? L"Band" : L"Body");
            }
        rowsXml += L"</table:table-row>";
        }

    if (!slide.m_title.empty())
        {
        frames += wxString::Format(
            L"<draw:frame draw:style-name=\"tsTitleFrameTable\" draw:layer=\"layout\" "
            L"svg:x=\"%s\" svg:y=\"%s\" svg:width=\"%s\" svg:height=\"%s\"><draw:text-box>"
            L"<text:p text:style-name=\"tsTableTitle\">%s</text:p></draw:text-box></draw:frame>",
            ToInches(TABLE_MARGIN_INCHES), ToInches(TABLE_MARGIN_INCHES), ToInches(tableWidth),
            ToInches(TABLE_TITLE_HEIGHT_INCHES), EscapeXml(slide.m_title));
        }

    // ODF text has no standard opacity, so the watermark goes behind the table in a paler
    // color, the same as its opacity over a white slide
    if (slide.m_watermark.has_value())
        {
        constexpr double POINTS_PER_INCH{ 72.0 };
        constexpr double WATERMARK_LINE_HEIGHT{ 1.5 };

        const ReportWatermarkLayout& watermark{ slide.m_watermark.value() };
        const auto blendChannel = [&watermark](const unsigned char channel)
        {
            return static_cast<unsigned char>(std::lround(
                (channel * watermark.m_opacity) + (wxALPHA_OPAQUE * (1 - watermark.m_opacity))));
        };
        const wxColour paleColor{ blendChannel(watermark.m_color.Red()),
                                  blendChannel(watermark.m_color.Green()),
                                  blendChannel(watermark.m_color.Blue()) };
        automaticStyles += wxString::Format(
            L"<style:style style:name=\"twm%zu\" style:family=\"paragraph\">"
            L"<style:paragraph-properties fo:text-align=\"center\"/>"
            L"<style:text-properties fo:font-size=\"%dpt\" fo:font-weight=\"bold\" "
            L"fo:color=\"%s\"/></style:style>",
            slideNumber, static_cast<int>(std::lround(watermark.m_fontSize)),
            ColorToOdfHex(paleColor));

        const double boxWidth{ watermark.m_length / POINTS_PER_INCH };
        const double boxHeight{ watermark.m_fontSize * WATERMARK_LINE_HEIGHT / POINTS_PER_INCH };
        // draw:transform turns the frame around its top-left corner, not its center
        const double radians{ watermark.m_angle * std::numbers::pi / 180 };
        const double cornerX{ (slide.m_slideWidth / 2) - (boxWidth / 2 * std::cos(radians)) -
                              (boxHeight / 2 * std::sin(radians)) };
        const double cornerY{ (slide.m_slideHeight / 2) + (boxWidth / 2 * std::sin(radians)) -
                              (boxHeight / 2 * std::cos(radians)) };
        frames += wxString::Format(
            L"<draw:frame draw:style-name=\"tsWatermarkFrame\" draw:layer=\"layout\" "
            L"svg:width=\"%s\" svg:height=\"%s\" "
            L"draw:transform=\"rotate (%s) translate (%s %s)\"><draw:text-box>"
            L"<text:p text:style-name=\"twm%zu\">%s</text:p></draw:text-box></draw:frame>",
            ToInches(boxWidth), ToInches(boxHeight), wxString::FromCDouble(radians, 6),
            ToInches(cornerX), ToInches(cornerY), slideNumber,
            EscapeXmlAttribute(watermark.m_label));
        }

    frames += wxString::Format(
        L"<draw:frame draw:layer=\"layout\" svg:x=\"%s\" svg:y=\"%s\" svg:width=\"%s\" "
        L"svg:height=\"%s\"><table:table>%s%s</table:table>%s</draw:frame>",
        ToInches(TABLE_MARGIN_INCHES), ToInches(slide.m_top), ToInches(tableWidth),
        ToInches(slide.GetTableHeight()), columnsXml, rowsXml,
        slide.m_title.empty() ?
            wxString{} :
            wxString::Format(L"<svg:title>%s</svg:title>", EscapeXml(slide.m_title)));
    return frames;
    }

//------------------------------------------------------
Wisteria::ReportOdpExport::ReportOdpExport(const std::vector<Canvas*>& canvases,
                                           const wxString& filePath,
                                           const OdpExportOptions& options)
    : ReportOdpExport(ReportExportPage::FromCanvases(canvases), filePath, options)
    {
    }

//------------------------------------------------------
Wisteria::ReportOdpExport::ReportOdpExport(const std::vector<ReportExportPage>& exportPages,
                                           const wxString& filePath,
                                           const OdpExportOptions& options)
    {
    if (filePath.empty())
        {
        return;
        }

    const double slideWidthInches{ options.GetSlideWidthInches() };
    const double slideHeightInches{ options.GetSlideHeightInches() };
    const wxSize slideDIPs{ options.GetSlideSizeDIPs() };

    // render every page
    std::vector<RenderedPage> rendered;
    rendered.reserve(exportPages.size());
    for (const auto& exportPage : exportPages)
        {
        const auto* const canvasHolder = std::get_if<Canvas*>(&exportPage.m_content);
        if (canvasHolder == nullptr)
            {
            for (auto& tableSlide :
                 BuildTableSlides(exportPage, slideWidthInches, slideHeightInches))
                {
                RenderedPage tablePage;
                tablePage.m_label = tableSlide.m_title;
                tablePage.m_table = std::move(tableSlide);
                rendered.push_back(std::move(tablePage));
                }
            continue;
            }
        Canvas* const canvas{ *canvasHolder };
        if (canvas == nullptr)
            {
            continue;
            }
        canvas->ApplyAutoAccessibilityAttributes();

        RenderedPage page;
        page.m_label = canvas->GetLabel();
        if (options.m_includeAccessibilityNotes)
            {
            page.m_notes = CollectAccessibilityText(canvas);
            }

        wxSize renderSize{ slideDIPs };
        if (!canvas->IsFittingToPageWhenPrinting())
            {
            const int naturalWidth{ canvas->GetCanvasMinWidthDIPs() };
            const int naturalHeight{ canvas->GetCanvasMinHeightDIPs() };
            if (naturalWidth > 0 && naturalHeight > 0)
                {
                renderSize = wxSize{ naturalWidth, naturalHeight };
                }
            }
        page.m_pixelWidth = renderSize.GetWidth();
        page.m_pixelHeight = renderSize.GetHeight();
        RenderCanvas(canvas, renderSize, page.m_svg, nullptr);
        rendered.push_back(std::move(page));
        }
    if (rendered.empty())
        {
        return;
        }

    const bool includeTitleSlide{ options.m_includeTitleSlide && !options.m_title.empty() };

    // only pages drawn as pictures have a file in the package
    std::vector<wxString> pictureFileNames;
    pictureFileNames.reserve(rendered.size());
    for (size_t pageIndex = 0; pageIndex < rendered.size(); ++pageIndex)
        {
        if (!rendered[pageIndex].m_table.has_value())
            {
            pictureFileNames.push_back(wxString::Format(L"image%zu.svg", pageIndex + 1));
            }
        }

    const wxString nowUtc{ wxDateTime::Now().ToUTC().Format(L"%Y-%m-%dT%H:%M:%S") };
    wxString metaXml{ wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        L"<office:document-meta "
        L"xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        L"xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        L"xmlns:meta=\"urn:oasis:names:tc:opendocument:xmlns:meta:1.0\" office:version=\"1.3\">"
        L"<office:meta><meta:generator>Wisteria-Dataviz</meta:generator>"
        L"<dc:title>%s</dc:title><dc:subject>%s</dc:subject><dc:creator>%s</dc:creator>"
        L"<meta:initial-creator>%s</meta:initial-creator>"
        L"<meta:creation-date>%s</meta:creation-date><dc:date>%s</dc:date>",
        EscapeXml(options.m_title), EscapeXml(options.m_subject), EscapeXml(options.m_author),
        EscapeXml(options.m_author), nowUtc, nowUtc) };
    if (!options.m_keywords.empty())
        {
        metaXml +=
            wxString::Format(L"<meta:keyword>%s</meta:keyword>", EscapeXml(options.m_keywords));
        }
    if (!options.m_publisher.empty())
        {
        metaXml +=
            wxString::Format(L"<meta:user-defined meta:name=\"Publisher\">%s</meta:user-defined>",
                             EscapeXml(options.m_publisher));
        }
    metaXml += L"</office:meta></office:document-meta>";

    const wxString transitionAttributes{ BuildTransitionAttributes(options) };

    const wxString stylesXml{ wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        L"<office:document-styles "
        L"xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        L"xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
        L"xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" "
        L"xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\" "
        L"xmlns:draw=\"urn:oasis:names:tc:opendocument:xmlns:drawing:1.0\" "
        L"xmlns:presentation=\"urn:oasis:names:tc:opendocument:xmlns:presentation:1.0\" "
        L"office:version=\"1.3\">"
        L"<office:styles><style:style style:name=\"dp1\" style:family=\"drawing-page\">"
        L"<style:drawing-page-properties draw:fill=\"solid\" draw:fill-color=\"#FFFFFF\"/>"
        L"</style:style></office:styles>"
        L"<office:automatic-styles><style:page-layout style:name=\"PM1\">"
        L"<style:page-layout-properties fo:margin-top=\"0in\" fo:margin-bottom=\"0in\" "
        L"fo:margin-left=\"0in\" fo:margin-right=\"0in\" fo:page-width=\"%s\" "
        L"fo:page-height=\"%s\" style:print-orientation=\"landscape\"/></style:page-layout>"
        L"</office:automatic-styles>"
        L"<office:master-styles><style:master-page style:name=\"Default\" "
        L"style:page-layout-name=\"PM1\" draw:style-name=\"dp1\"/></office:master-styles>"
        L"</office:document-styles>",
        ReportOdpExport::ToInches(slideWidthInches),
        ReportOdpExport::ToInches(slideHeightInches)) };

    // LibreOffice only honors presentation:transition-* when the drawing-page style is
    // declared as an automatic style in content.xml, not a common style in styles.xml.
    // Content pages get their own automatic style here rather than reusing dp1.
    wxString automaticStyles{ wxString::Format(
        L"<style:style style:name=\"gr1\" style:family=\"graphic\">"
        L"<style:graphic-properties draw:fill=\"none\" draw:stroke=\"none\"/></style:style>"
        L"<style:style style:name=\"dpContent\" style:family=\"drawing-page\">"
        L"<style:drawing-page-properties draw:fill=\"solid\" draw:fill-color=\"#FFFFFF\"%s/>"
        L"</style:style>"
        L"<style:style style:name=\"tsTitleFrameTable\" style:family=\"graphic\">"
        L"<style:graphic-properties draw:fill=\"none\" draw:stroke=\"none\" "
        L"draw:textarea-vertical-align=\"middle\"/></style:style>"
        L"<style:style style:name=\"tsWatermarkFrame\" style:family=\"graphic\">"
        L"<style:graphic-properties draw:fill=\"none\" draw:stroke=\"none\" "
        L"draw:textarea-vertical-align=\"middle\" fo:wrap-option=\"no-wrap\"/></style:style>",
        transitionAttributes) };
    automaticStyles += BuildTableStyles();
    const wxString titleSlideXml{ includeTitleSlide ?
                                      BuildTitleSlideXml(options, slideWidthInches,
                                                         slideHeightInches, transitionAttributes,
                                                         automaticStyles) :
                                      wxString{} };

    wxString pagesXml;
    for (size_t pageIndex = 0; pageIndex < rendered.size(); ++pageIndex)
        {
        const size_t oneBased{ pageIndex + 1 };
        const RenderedPage& page{ rendered[pageIndex] };

        if (page.m_table.has_value())
            {
            pagesXml += wxString::Format(
                L"<draw:page draw:name=\"%s\" draw:style-name=\"dpContent\" "
                L"draw:master-page-name=\"Default\">%s</draw:page>",
                EscapeXmlAttribute(wxString::Format(_(L"Page %zu"), oneBased)),
                BuildTableSlideXml(page.m_table.value(), oneBased, automaticStyles));
            continue;
            }

        const wxString altText{ page.m_notes.empty() ? page.m_label :
                                                       page.m_notes.Left(MAX_ALT_TEXT_LENGTH) };
        const wxString slideTitle{ !page.m_label.empty() ?
                                       page.m_label :
                                       wxString::Format(_(L"Page %zu"), oneBased) };
        const wxString placement{ BuildPicturePlacementAttributes(
            slideWidthInches, slideHeightInches, page.m_pixelWidth, page.m_pixelHeight) };

        // draw:name must be unique across pages, so it's tied to the page number rather
        // than the (possibly duplicated) canvas label used for svg:title below
        pagesXml += wxString::Format(
            L"<draw:page draw:name=\"%s\" draw:style-name=\"dpContent\" "
            L"draw:master-page-name=\"Default\">"
            L"<draw:frame draw:style-name=\"gr1\" draw:layer=\"layout\" %s>"
            L"<svg:title>%s</svg:title><svg:desc>%s</svg:desc>"
            L"<draw:image xlink:href=\"Pictures/%s\" xlink:type=\"simple\" xlink:show=\"embed\" "
            L"xlink:actuate=\"onLoad\" draw:mime-type=\"image/svg+xml\"/></draw:frame>%s"
            L"</draw:page>",
            EscapeXmlAttribute(wxString::Format(_(L"Page %zu"), oneBased)), placement,
            EscapeXml(slideTitle), EscapeXml(altText), wxString::Format(L"image%zu.svg", oneBased),
            BuildNotesXml(page.m_notes));
        }

    const wxString contentXml{ wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        L"<office:document-content "
        L"xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        L"xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
        L"xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
        L"xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\" "
        L"xmlns:draw=\"urn:oasis:names:tc:opendocument:xmlns:drawing:1.0\" "
        L"xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" "
        L"xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        L"xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\" "
        L"xmlns:presentation=\"urn:oasis:names:tc:opendocument:xmlns:presentation:1.0\" "
        L"office:version=\"1.3\">"
        L"<office:automatic-styles>%s</office:automatic-styles>"
        L"<office:body><office:presentation>%s%s</office:presentation></office:body>"
        L"</office:document-content>",
        automaticStyles, titleSlideXml, pagesXml) };

    // write the package
    wxFFileOutputStream fileStream{ filePath };
    if (!fileStream.IsOk())
        {
        Settings::ReportError(
            wxString::Format(_(L"Failed to save ODP report to \"%s\"."), filePath),
            _(L"Export Error"));
        return;
        }
    wxZipOutputStream zipStream{ fileStream };

    const auto addStored = [&zipStream](const wxString& partPath, const wxString& content) -> bool
    {
        zipStream.SetLevel(0);
        if (!zipStream.PutNextEntry(new wxZipEntry{ partPath }))
            {
            return false;
            }
        const wxScopedCharBuffer utf8{ content.utf8_str() };
        zipStream.Write(utf8.data(), utf8.length());
        return zipStream.CloseEntry();
    };
    const auto addText = [&zipStream](const wxString& partPath, const wxString& content) -> bool
    {
        zipStream.SetLevel(6);
        if (!zipStream.PutNextEntry(new wxZipEntry{ partPath }))
            {
            return false;
            }
        const wxScopedCharBuffer utf8{ content.utf8_str() };
        zipStream.Write(utf8.data(), utf8.length());
        return zipStream.CloseEntry();
    };

    bool ok{ addStored(L"mimetype", wxString{ MIME_TYPE }) &&
             addText(L"META-INF/manifest.xml", BuildManifestXml(pictureFileNames)) &&
             addText(L"content.xml", contentXml) && addText(L"styles.xml", stylesXml) &&
             addText(L"meta.xml", metaXml) };

    for (size_t pageIndex = 0; pageIndex < rendered.size(); ++pageIndex)
        {
        if (!rendered[pageIndex].m_table.has_value())
            {
            ok = addText(wxString::Format(L"Pictures/image%zu.svg", pageIndex + 1),
                         rendered[pageIndex].m_svg) &&
                 ok;
            }
        }

    const bool closedOk{ zipStream.Close() && fileStream.Close() };
    if (!ok || !closedOk)
        {
        Settings::ReportError(
            wxString::Format(_(L"Failed to save ODP report to \"%s\"."), filePath),
            _(L"Export Error"));
        }
    }
