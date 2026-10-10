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
    return wxString::Format(L"%.4fin", value);
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

    // drawing-page background: gradient when themed, plain white otherwise.
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
Wisteria::ReportOdpExport::ReportOdpExport(const std::vector<Canvas*>& canvases,
                                           const wxString& filePath,
                                           const OdpExportOptions& options)
    {
    if (filePath.empty())
        {
        return;
        }

    std::vector<Canvas*> pages;
    pages.reserve(canvases.size());
    for (auto* canvas : canvases)
        {
        if (canvas != nullptr)
            {
            pages.push_back(canvas);
            }
        }
    if (pages.empty())
        {
        return;
        }

    for (auto* canvas : pages)
        {
        canvas->ApplyAutoAccessibilityAttributes();
        }

    const double slideWidthInches{ options.GetSlideWidthInches() };
    const double slideHeightInches{ options.GetSlideHeightInches() };
    const wxSize slideDIPs{ options.GetSlideSizeDIPs() };

    // render every page
    std::vector<RenderedPage> rendered;
    rendered.reserve(pages.size());
    for (auto* canvas : pages)
        {
        RenderedPage page;
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

    const bool includeTitleSlide{ options.m_includeTitleSlide && !options.m_title.empty() };

    std::vector<wxString> pictureFileNames;
    pictureFileNames.reserve(pages.size());
    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        pictureFileNames.push_back(wxString::Format(L"image%zu.svg", pageIndex + 1));
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
        L"</style:style>",
        transitionAttributes) };
    const wxString titleSlideXml{ includeTitleSlide ?
                                      BuildTitleSlideXml(options, slideWidthInches,
                                                         slideHeightInches, transitionAttributes,
                                                         automaticStyles) :
                                      wxString{} };

    wxString pagesXml;
    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        const size_t oneBased{ pageIndex + 1 };
        const RenderedPage& page{ rendered[pageIndex] };

        const wxString altText{ page.m_notes.empty() ? pages[pageIndex]->GetLabel() :
                                                       page.m_notes.Left(MAX_ALT_TEXT_LENGTH) };
        const wxString slideTitle{ !pages[pageIndex]->GetLabel().empty() ?
                                       pages[pageIndex]->GetLabel() :
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
            EscapeXml(slideTitle), EscapeXml(altText), pictureFileNames[pageIndex],
            BuildNotesXml(page.m_notes));
        }

    const wxString contentXml{ wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        L"<office:document-content "
        L"xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        L"xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
        L"xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
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

    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        ok = addText(L"Pictures/" + pictureFileNames[pageIndex], rendered[pageIndex].m_svg) && ok;
        }

    const bool closedOk{ zipStream.Close() && fileStream.Close() };
    if (!ok || !closedOk)
        {
        Settings::ReportError(
            wxString::Format(_(L"Failed to save ODP report to \"%s\"."), filePath),
            _(L"Export Error"));
        }
    }
