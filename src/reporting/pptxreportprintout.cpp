///////////////////////////////////////////////////////////////////////////////
// Name:        pptxreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pptxreportprintout.h"
#include "../base/colorschemenames.h"
#include "../base/graphitems.h"
#include "../base/settings.h"
#include "../math/safe_math.h"
#include "reportprintout.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <wx/arrstr.h>
#include <wx/datetime.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/dcsvg.h>
#include <wx/graphics.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

// fixed OPC parts
const std::wstring_view Wisteria::ReportPowerPointExport::THEME_XML{
    LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Office Theme"><a:themeElements><a:clrScheme name="Office"><a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1><a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1><a:dk2><a:srgbClr val="44546A"/></a:dk2><a:lt2><a:srgbClr val="E7E6E6"/></a:lt2><a:accent1><a:srgbClr val="4472C4"/></a:accent1><a:accent2><a:srgbClr val="ED7D31"/></a:accent2><a:accent3><a:srgbClr val="A5A5A5"/></a:accent3><a:accent4><a:srgbClr val="FFC000"/></a:accent4><a:accent5><a:srgbClr val="5B9BD5"/></a:accent5><a:accent6><a:srgbClr val="70AD47"/></a:accent6><a:hlink><a:srgbClr val="0563C1"/></a:hlink><a:folHlink><a:srgbClr val="954F72"/></a:folHlink></a:clrScheme><a:fontScheme name="Office"><a:majorFont><a:latin typeface="Calibri Light"/><a:ea typeface=""/><a:cs typeface=""/></a:majorFont><a:minorFont><a:latin typeface="Calibri"/><a:ea typeface=""/><a:cs typeface=""/></a:minorFont></a:fontScheme><a:fmtScheme name="Office"><a:fillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"><a:lumMod val="110000"/><a:satMod val="105000"/><a:tint val="67000"/></a:schemeClr></a:gs><a:gs pos="50000"><a:schemeClr val="phClr"><a:lumMod val="105000"/><a:satMod val="103000"/><a:tint val="73000"/></a:schemeClr></a:gs><a:gs pos="100000"><a:schemeClr val="phClr"><a:lumMod val="105000"/><a:satMod val="109000"/><a:tint val="81000"/></a:schemeClr></a:gs></a:gsLst><a:lin ang="5400000" scaled="0"/></a:gradFill><a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"><a:satMod val="103000"/><a:lumMod val="102000"/><a:tint val="94000"/></a:schemeClr></a:gs><a:gs pos="50000"><a:schemeClr val="phClr"><a:satMod val="110000"/><a:lumMod val="100000"/><a:shade val="100000"/></a:schemeClr></a:gs><a:gs pos="100000"><a:schemeClr val="phClr"><a:lumMod val="99000"/><a:satMod val="120000"/><a:shade val="78000"/></a:schemeClr></a:gs></a:gsLst><a:lin ang="5400000" scaled="0"/></a:gradFill></a:fillStyleLst><a:lnStyleLst><a:ln w="6350" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/><a:miter lim="800000"/></a:ln><a:ln w="12700" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/><a:miter lim="800000"/></a:ln><a:ln w="19050" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/><a:miter lim="800000"/></a:ln></a:lnStyleLst><a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst><a:bgFillStyleLst><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:solidFill><a:schemeClr val="phClr"><a:tint val="95000"/><a:satMod val="170000"/></a:schemeClr></a:solidFill><a:solidFill><a:schemeClr val="phClr"><a:tint val="93000"/><a:satMod val="150000"/><a:shade val="98000"/><a:lumMod val="102000"/></a:schemeClr></a:solidFill></a:bgFillStyleLst></a:fmtScheme></a:themeElements></a:theme>)"
};

const std::wstring_view Wisteria::ReportPowerPointExport::SLIDE_MASTER_XML{
    LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:cSld><p:bg><p:bgRef idx="1001"><a:schemeClr val="bg1"/></p:bgRef></p:bg><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/><p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst><p:txStyles><p:titleStyle><a:lvl1pPr algn="ctr" defTabSz="914400" rtl="0" eaLnBrk="1" latinLnBrk="0" hangingPunct="1"><a:spcBef><a:spcPct val="0"/></a:spcBef><a:buNone/><a:defRPr sz="4400" kern="1200"><a:solidFill><a:schemeClr val="tx1"/></a:solidFill><a:latin typeface="+mj-lt"/></a:defRPr></a:lvl1pPr></p:titleStyle><p:bodyStyle><a:lvl1pPr marL="342900" indent="-342900" algn="l" defTabSz="914400" rtl="0" eaLnBrk="1" latinLnBrk="0" hangingPunct="1"><a:spcBef><a:spcPct val="20000"/></a:spcBef><a:buFont typeface="Arial" pitchFamily="34" charset="0"/><a:buChar char="&#8226;"/><a:defRPr sz="2800" kern="1200"><a:solidFill><a:schemeClr val="tx1"/></a:solidFill><a:latin typeface="+mn-lt"/></a:defRPr></a:lvl1pPr></p:bodyStyle><p:otherStyle><a:defPPr><a:defRPr lang="en-US"/></a:defPPr><a:lvl1pPr marL="0" algn="l" defTabSz="914400" rtl="0" eaLnBrk="1" latinLnBrk="0" hangingPunct="1"><a:defRPr sz="1800" kern="1200"><a:solidFill><a:schemeClr val="tx1"/></a:solidFill><a:latin typeface="+mn-lt"/></a:defRPr></a:lvl1pPr></p:otherStyle></p:txStyles></p:sldMaster>)"
};

const std::wstring_view Wisteria::ReportPowerPointExport::SLIDE_LAYOUT_XML{
    LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1"><p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>)"
};

const std::wstring_view Wisteria::ReportPowerPointExport::NOTES_MASTER_XML{
    LR"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:notesMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"><p:cSld><p:bg><p:bgRef idx="1001"><a:schemeClr val="bg1"/></p:bgRef></p:bg><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr><p:sp><p:nvSpPr><p:cNvPr id="2" name="Notes Placeholder 1"/><p:cNvSpPr><a:spLocks noGrp="1"/></p:cNvSpPr><p:nvPr><p:ph type="body" idx="1"/></p:nvPr></p:nvSpPr><p:spPr><a:xfrm><a:off x="685800" y="1143000"/><a:ext cx="5486400" cy="6858000"/></a:xfrm><a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr><p:txBody><a:bodyPr/><a:lstStyle/><a:p><a:endParaRPr lang="en-US"/></a:p></p:txBody></p:sp></p:spTree></p:cSld><p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/><p:notesStyle><a:lvl1pPr marL="0" algn="l" defTabSz="914400" rtl="0" eaLnBrk="1" latinLnBrk="0" hangingPunct="1"><a:defRPr sz="1200" kern="1200"><a:solidFill><a:schemeClr val="tx1"/></a:solidFill><a:latin typeface="+mn-lt"/></a:defRPr></a:lvl1pPr></p:notesStyle></p:notesMaster>)"
};

//------------------------------------------------------
wxSize Wisteria::PowerPointExportOptions::GetSlideSizeEMU() const
    {
    switch (m_slideSize)
        {
    case SlideSize::Standard4x3:
        return { SLIDE_WIDTH_4X3_EMU, SLIDE_HEIGHT_EMU };
    case SlideSize::Custom:
        {
        const double widthInches{ std::clamp(m_customWidthInches, 1.0, MAX_SLIDE_INCHES) };
        const double heightInches{ std::clamp(m_customHeightInches, 1.0, MAX_SLIDE_INCHES) };
        return { static_cast<int>(std::llround(widthInches * EMU_PER_INCH)),
                 static_cast<int>(std::llround(heightInches * EMU_PER_INCH)) };
        }
    case SlideSize::Widescreen16x9:
    default:
        return { SLIDE_WIDTH_16X9_EMU, SLIDE_HEIGHT_EMU };
        }
    }

//------------------------------------------------------
wxSize Wisteria::PowerPointExportOptions::GetSlideSizeDIPs() const
    {
    constexpr double EMU_PER_DIP{ EMU_PER_INCH / 96.0 };
    const wxSize emuSize{ GetSlideSizeEMU() };
    return { static_cast<int>(std::llround(emuSize.GetWidth() / EMU_PER_DIP)),
             static_cast<int>(std::llround(emuSize.GetHeight() / EMU_PER_DIP)) };
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::EscapeXml(const wxString& str)
    {
    wxString result;
    result.reserve(static_cast<size_t>(str.length() * 1.1));
    for (size_t charIndex = 0; charIndex < str.length(); ++charIndex)
        {
        const wxUniChar currentChar = str[charIndex];
        if (currentChar == L'&')
            {
            result += L"&amp;";
            }
        else if (currentChar == L'<')
            {
            result += L"&lt;";
            }
        else if (currentChar == L'>')
            {
            result += L"&gt;";
            }
        else if (currentChar == L'"')
            {
            result += L"&quot;";
            }
        else if (currentChar == L'\'')
            {
            result += L"&apos;";
            }
        else if (currentChar >= 0x20 || currentChar == L'\t' || currentChar == L'\n' ||
                 currentChar == L'\r')
            {
            result += currentChar;
            }
        }
    return result;
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::EscapeXmlAttribute(const wxString& str)
    {
    wxString collapsed{ str };
    collapsed.Replace(L"\r\n", L" ");
    collapsed.Replace(L"\r", L" ");
    collapsed.Replace(L"\n", L" ");
    collapsed.Replace(L"\t", L" ");
    while (collapsed.Replace(L"  ", L" ") > 0)
        {
        }
    return EscapeXml(collapsed.Trim(true).Trim(false));
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::BuildRelationshipsXml(
    const std::vector<std::tuple<wxString, wxString, wxString>>& relationships)
    {
    wxString xml{ L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                  L"<Relationships "
                  L"xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">" };
    for (const auto& [relId, relType, relTarget] : relationships)
        {
        xml += wxString::Format(L"<Relationship Id=\"%s\" Type=\"%s\" Target=\"%s\"/>", relId,
                                relType, relTarget);
        }
    xml += L"</Relationships>";
    return xml;
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::CollectAccessibilityText(Canvas* canvas)
    {
    if (canvas == nullptr)
        {
        return {};
        }

    wxArrayString lines;
    const auto addTitles = [&lines](const std::vector<GraphItems::Label>& titles)
    {
        for (const auto& title : titles)
            {
            if (!title.GetText().empty())
                {
                lines.Add(title.GetText());
                }
            }
    };
    addTitles(canvas->GetTopTitles());
    addTitles(canvas->GetLeftTitles());
    addTitles(canvas->GetRightTitles());
    addTitles(canvas->GetBottomTitles());

    const auto [rowCount, columnCount] = canvas->GetFixedObjectsGridSize();
    for (size_t row = 0; row < rowCount; ++row)
        {
        for (size_t column = 0; column < columnCount; ++column)
            {
            const auto fixedObject = canvas->GetFixedObject(row, column);
            if (fixedObject == nullptr)
                {
                continue;
                }
            wxString label = fixedObject->GetAccessibilityAttributes().GetAriaLabel();
            if (label.empty())
                {
                label = fixedObject->GetAutoAccessibilityAttributes().GetAriaLabel();
                }
            if (!label.empty())
                {
                wxString buffer;
                Wisteria::GraphItems::GraphItemBase::AddAccessibilityAttribute(buffer, label,
                                                                               wxString{});
                lines.Add(buffer);
                }
            }
        }

    wxString result;
    for (size_t lineIndex = 0; lineIndex < lines.GetCount(); ++lineIndex)
        {
        if (lineIndex > 0)
            {
            result += L"\n\n";
            }
        result += lines[lineIndex];
        }
    return result;
    }

//------------------------------------------------------
void Wisteria::ReportPowerPointExport::RenderCanvas(Canvas* canvas, const wxSize renderSize,
                                                    wxString& svgOut, wxMemoryBuffer& pngOut)
    {
    const wxSize safeSize{ std::max(1, renderSize.GetWidth()),
                           std::max(1, renderSize.GetHeight()) };

    const wxWindowUpdateLocker updateLocker{ canvas };

    // temporarily lay the canvas out at the render size
    const int origMinWidth{ canvas->GetCanvasMinWidthDIPs() };
    const int origMinHeight{ canvas->GetCanvasMinHeightDIPs() };
    const wxSize origSize{ canvas->GetSize() };
    if (canvas->IsFittingToPageWhenPrinting())
        {
        canvas->SetCanvasMinWidthDIPs(safeSize.GetWidth());
        canvas->SetCanvasMinHeightDIPs(safeSize.GetHeight());
        // calling SetSize before CalcRowDimensions() is needed because some
        // internals look at the window size rather than the min width/height
        canvas->SetSize(canvas->FromDIP(safeSize));
        canvas->CalcRowDimensions();
        canvas->SetSize(canvas->FromDIP(safeSize));
        }

        // PNG first, mirroring the raster path in Canvas::Save so the layout the
        // resize produced is drawn without an intervening re-measure.
        {
        wxBitmap exportBmp;
        exportBmp.CreateWithDIPSize(safeSize, canvas->GetDPIScaleFactor());
        GraphItems::Image::SetOpacity(exportBmp, wxALPHA_OPAQUE, false);

            {
            wxMemoryDC memDc{ exportBmp };
            memDc.Clear();
            const wxEventBlocker blocker{ canvas };
#ifdef __WXMSW__
            wxGraphicsContext* graphicsContext{ nullptr };
            auto* d2dRenderer = wxGraphicsRenderer::GetDirect2DRenderer();
            if (d2dRenderer != nullptr)
                {
                graphicsContext = d2dRenderer->CreateContext(memDc);
                }
            if (graphicsContext != nullptr)
                {
                wxGCDC gcdc{ graphicsContext };
                canvas->OnDraw(gcdc);
                canvas->DrawWatermarkLabel(gcdc);
                }
            else
                {
                wxGCDC gcdc{ memDc };
                canvas->OnDraw(gcdc);
                canvas->DrawWatermarkLabel(gcdc);
                }
#else
            wxGCDC gcdc{ memDc };
            canvas->OnDraw(gcdc);
            canvas->DrawWatermarkLabel(gcdc);
#endif
            memDc.SelectObject(wxNullBitmap);
            }

        wxImage exportImg{ exportBmp.ConvertToImage() };
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES);
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONX,
                            Settings::GetImageResolutionDPI().GetWidth());
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONY,
                            Settings::GetImageResolutionDPI().GetHeight());
        exportImg.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
        if (!canvas->GetLabel().empty())
            {
            exportImg.SetOption(wxIMAGE_OPTION_PNG_DESCRIPTION, canvas->GetLabel());
            }

        wxMemoryOutputStream pngStream;
        exportImg.SaveFile(pngStream, wxBITMAP_TYPE_PNG);

        const size_t pngLength{ static_cast<size_t>(pngStream.GetLength()) };
        pngOut = wxMemoryBuffer(pngLength);
        pngStream.CopyTo(pngOut.GetData(), pngLength);
        pngOut.SetDataLen(pngLength);
        }

        // SVG
        {
        wxSVGFileDC svgDc{ wxString{}, safeSize.GetWidth(), safeSize.GetHeight(), wxSVG_DEFAULT_DPI,
                           canvas->GetLabel() };
        svgDc.SetBitmapHandler(new wxSVGBitmapEmbedHandler{});
        const wxEventBlocker blocker{ canvas };
        canvas->CalcAllSizes(svgDc);
        canvas->OnDraw(svgDc);
        canvas->DrawWatermarkLabel(svgDc);
        svgOut = svgDc.GetSVGDocument();
        }

    // restore the canvas's original size and layout
    if (canvas->IsFittingToPageWhenPrinting())
        {
        canvas->SetCanvasMinWidthDIPs(origMinWidth);
        canvas->SetCanvasMinHeightDIPs(origMinHeight);
        canvas->SetSize(origSize);
        canvas->CalcRowDimensions();
        canvas->SetSize(origSize);
        }

        // restore the on-screen measurements
        {
        wxGCDC screenDc{ canvas };
        canvas->CalcAllSizes(screenDc);
        }
    canvas->ResetResizeDelay();
    canvas->ZoomReset();
    canvas->SendSizeEvent();
    canvas->Refresh();
    }

//------------------------------------------------------
wxString
Wisteria::ReportPowerPointExport::BuildTransitionXml(const PowerPointExportOptions& options)
    {
    using Transition = PowerPointExportOptions::Transition;
    using Speed = PowerPointExportOptions::TransitionSpeed;

    const bool autoAdvance{ options.m_advanceAutomatically };
    if (options.m_transition == Transition::None && !autoAdvance)
        {
        return {};
        }

    wxString speed{ L"med" };
    if (options.m_transitionSpeed == Speed::Slow)
        {
        speed = L"slow";
        }
    else if (options.m_transitionSpeed == Speed::Fast)
        {
        speed = L"fast";
        }

    wxString attribs{ wxString::Format(L" spd=\"%s\"", speed) };
    attribs += options.m_advanceOnClick ? L" advClick=\"1\"" : L" advClick=\"0\"";
    if (autoAdvance)
        {
        attribs += wxString::Format(L" advTm=\"%d\"", std::max(0, options.m_advanceSeconds) *
                                                          MILLISECONDS_PER_SECOND);
        }

    if (options.m_transition == Transition::Morph)
        {
        // Morph needs the 2015+ transition extension. Emit it through MCE
        // AlternateContent with a Fade fallback for clients without it.
        return wxString::Format(
            L"<mc:AlternateContent "
            L"xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\">"
            L"<mc:Choice "
            L"xmlns:p159=\"http://schemas.microsoft.com/office/powerpoint/2015/09/main\" "
            L"Requires=\"p159\">"
            L"<p:transition%s><p159:morph option=\"byObject\"/></p:transition>"
            L"</mc:Choice>"
            L"<mc:Fallback><p:transition%s><p:fade/></p:transition></mc:Fallback>"
            L"</mc:AlternateContent>",
            attribs, attribs);
        }

    wxString effect;
    switch (options.m_transition)
        {
    case Transition::Fade:
        effect = L"<p:fade/>";
        break;
    case Transition::Push:
        effect = L"<p:push dir=\"l\"/>";
        break;
    case Transition::Wipe:
        effect = L"<p:wipe dir=\"l\"/>";
        break;
    case Transition::Split:
        effect = L"<p:split orient=\"horz\" dir=\"out\"/>";
        break;
    case Transition::Cut:
        effect = L"<p:cut/>";
        break;
    case Transition::None:
    case Transition::Morph:
    default:
        effect = wxString{};
        break;
        }

    return wxString::Format(L"<p:transition%s>%s</p:transition>", attribs, effect);
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::BuildPicturePlacementXml(const long long slideCx,
                                                                    const long long slideCy,
                                                                    const int imageWidth,
                                                                    const int imageHeight)
    {
    long long offsetX{ 0 };
    long long offsetY{ 0 };
    long long extentCx{ slideCx };
    long long extentCy{ slideCy };

    if (imageWidth > 0 && imageHeight > 0)
        {
        const double slideAspect{ safe_divide(static_cast<double>(slideCx),
                                              static_cast<double>(slideCy)) };
        const double imageAspect{ safe_divide(static_cast<double>(imageWidth),
                                              static_cast<double>(imageHeight)) };
        if (imageAspect > slideAspect)
            {
            extentCx = slideCx;
            extentCy = std::llround(
                safe_divide(static_cast<double>(slideCx) * static_cast<double>(imageHeight),
                            static_cast<double>(imageWidth)));
            offsetY = (slideCy - extentCy) / 2;
            }
        else
            {
            extentCy = slideCy;
            extentCx = std::llround(
                safe_divide(static_cast<double>(slideCy) * static_cast<double>(imageWidth),
                            static_cast<double>(imageHeight)));
            offsetX = (slideCx - extentCx) / 2;
            }
        }

    return wxString::Format(
        L"<a:off x=\"%s\" y=\"%s\"/><a:ext cx=\"%s\" cy=\"%s\"/>",
        wxString{ std::to_wstring(offsetX) }, wxString{ std::to_wstring(offsetY) },
        wxString{ std::to_wstring(extentCx) }, wxString{ std::to_wstring(extentCy) });
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::ColorToHex(const wxColour& color)
    {
    return color.GetAsString(wxC2S_HTML_SYNTAX).Mid(1).Upper();
    }

//------------------------------------------------------
wxString Wisteria::ReportPowerPointExport::BuildTitleSlideXml(
    const PowerPointExportOptions& options, const long long slideCx, const long long slideCy,
    const wxString& transitionXml)
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

    const auto solidFillXml = [](const wxColour& color, const int alphaPercent) -> wxString
    {
        return (alphaPercent >= 100) ?
                   wxString::Format(L"<a:solidFill><a:srgbClr val=\"%s\"/></a:solidFill>",
                                    ColorToHex(color)) :
                   wxString::Format(
                       L"<a:solidFill><a:srgbClr val=\"%s\"><a:alpha val=\"%d\"/></a:srgbClr>"
                       L"</a:solidFill>",
                       ColorToHex(color), alphaPercent * ALPHA_UNITS_PER_PERCENT);
    };

    wxArrayString subtitleLines;
    if (!options.m_author.empty())
        {
        subtitleLines.Add(options.m_author);
        }
    const bool hasSubtitle{ !subtitleLines.empty() };

    const long long marginX{ std::llround(static_cast<double>(slideCx) *
                                          (isThemed ? 0.12 : 0.08)) };
    const long long boxWidth{ slideCx - (marginX * 2) };
    const long long titleY{ std::llround(static_cast<double>(slideCy) *
                                         (hasSubtitle ? 0.38 : 0.42)) };
    const long long titleHeight{ std::llround(static_cast<double>(slideCy) * 0.18) };

    const wxString titleBox{ wxString::Format(
        L"<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Title\"/>"
        L"<p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr/></p:nvSpPr>"
        L"<p:spPr><a:xfrm><a:off x=\"%s\" y=\"%s\"/><a:ext cx=\"%s\" cy=\"%s\"/></a:xfrm>"
        L"<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr>"
        L"<p:txBody><a:bodyPr anchor=\"ctr\" "
        L"wrap=\"square\"><a:normAutofit/></a:bodyPr><a:lstStyle/>"
        L"<a:p><a:pPr algn=\"ctr\"/><a:r><a:rPr lang=\"en-US\" sz=\"4400\" b=\"1\">%s</a:rPr>"
        L"<a:t>%s</a:t></a:r></a:p></p:txBody></p:sp>",
        wxString{ std::to_wstring(marginX) }, wxString{ std::to_wstring(titleY) },
        wxString{ std::to_wstring(boxWidth) }, wxString{ std::to_wstring(titleHeight) },
        isThemed ? solidFillXml(titleColor, 100) : wxString{}, EscapeXml(options.m_title)) };

    wxString subtitleBox;
    if (hasSubtitle)
        {
        wxString subtitleParas;
        for (size_t lineIndex = 0; lineIndex < subtitleLines.GetCount(); ++lineIndex)
            {
            subtitleParas += wxString::Format(
                L"<a:p><a:pPr algn=\"ctr\"/><a:r><a:rPr lang=\"en-US\" sz=\"2000\">%s</a:rPr>"
                L"<a:t>%s</a:t></a:r></a:p>",
                isThemed ? solidFillXml(titleColor, 100) : wxString{},
                EscapeXml(subtitleLines[lineIndex]));
            }
        const long long subtitleY{ std::llround(static_cast<double>(slideCy) *
                                                (isThemed ? 0.64 : 0.60)) };
        const long long subtitleHeight{ std::llround(static_cast<double>(slideCy) * 0.22) };
        subtitleBox = wxString::Format(
            L"<p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Subtitle\"/>"
            L"<p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr/></p:nvSpPr>"
            L"<p:spPr><a:xfrm><a:off x=\"%s\" y=\"%s\"/><a:ext cx=\"%s\" cy=\"%s\"/></a:xfrm>"
            L"<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr>"
            L"<p:txBody><a:bodyPr anchor=\"t\" wrap=\"square\"><a:normAutofit/></a:bodyPr>"
            L"<a:lstStyle/>%s</p:txBody></p:sp>",
            wxString{ std::to_wstring(marginX) }, wxString{ std::to_wstring(subtitleY) },
            wxString{ std::to_wstring(boxWidth) }, wxString{ std::to_wstring(subtitleHeight) },
            subtitleParas);
        }

    // thin accent rule centered beneath the title, only for themed slides
    wxString titleRuleXml;
    if (isThemed)
        {
        const long long ruleWidth{ std::llround(static_cast<double>(slideCx) * 0.10) };
        const long long ruleHeight{ std::llround(static_cast<double>(slideCy) * 0.006) };
        const long long ruleX{ (slideCx - ruleWidth) / 2 };
        const long long ruleY{ titleY + titleHeight +
                               std::llround(static_cast<double>(slideCy) * 0.015) };
        titleRuleXml = wxString::Format(
            L"<p:sp><p:nvSpPr><p:cNvPr id=\"7\" name=\"Title Rule\"/>"
            L"<p:cNvSpPr/><p:nvPr/></p:nvSpPr>"
            L"<p:spPr><a:xfrm><a:off x=\"%s\" y=\"%s\"/><a:ext cx=\"%s\" cy=\"%s\"/></a:xfrm>"
            L"<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>%s<a:ln><a:noFill/></a:ln></p:spPr>"
            L"</p:sp>",
            wxString{ std::to_wstring(ruleX) }, wxString{ std::to_wstring(ruleY) },
            wxString{ std::to_wstring(ruleWidth) }, wxString{ std::to_wstring(ruleHeight) },
            solidFillXml(accentColor, 100));
        }

    wxString publisherBox;
    if (!options.m_publisher.empty())
        {
        const long long publisherY{ std::llround(static_cast<double>(slideCy) * 0.92) };
        const long long publisherHeight{ std::llround(static_cast<double>(slideCy) * 0.06) };
        publisherBox = wxString::Format(
            L"<p:sp><p:nvSpPr><p:cNvPr id=\"4\" name=\"Publisher\"/>"
            L"<p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr/></p:nvSpPr>"
            L"<p:spPr><a:xfrm><a:off x=\"%s\" y=\"%s\"/><a:ext cx=\"%s\" cy=\"%s\"/></a:xfrm>"
            L"<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr>"
            L"<p:txBody><a:bodyPr anchor=\"b\" "
            L"wrap=\"square\"><a:normAutofit/></a:bodyPr><a:lstStyle/>"
            L"<a:p><a:pPr algn=\"ctr\"/><a:r><a:rPr lang=\"en-US\" sz=\"1200\">%s</a:rPr>"
            L"<a:t>%s</a:t></a:r></a:p></p:txBody></p:sp>",
            wxString{ std::to_wstring(marginX) }, wxString{ std::to_wstring(publisherY) },
            wxString{ std::to_wstring(boxWidth) }, wxString{ std::to_wstring(publisherHeight) },
            isThemed ? solidFillXml(titleColor, 100) : wxString{}, EscapeXml(options.m_publisher));
        }

    // diagonal gradient background between the theme's first two colors
    wxString bgXml;
    if (isThemed)
        {
        bgXml = wxString::Format(L"<p:bg><p:bgPr><a:gradFill rotWithShape=\"1\"><a:gsLst>"
                                 L"<a:gs pos=\"0\"><a:srgbClr val=\"%s\"/></a:gs>"
                                 L"<a:gs pos=\"100000\"><a:srgbClr val=\"%s\"/></a:gs>"
                                 L"</a:gsLst><a:lin ang=\"2700000\" scaled=\"1\"/></a:gradFill>"
                                 L"<a:effectLst/></p:bgPr></p:bg>",
                                 ColorToHex(bgColor1), ColorToHex(bgColor2));
        }

    // large, subtle accent circle bleeding off the top-right corner
    wxString accentCircleXml;
    if (isThemed)
        {
        const long long circleSize{ std::llround(static_cast<double>(slideCy) * 0.95) };
        const long long circleX{ std::llround(static_cast<double>(slideCx) * 0.60) };
        const long long circleY{ -std::llround(static_cast<double>(slideCy) * 0.40) };
        accentCircleXml = wxString::Format(
            L"<p:sp><p:nvSpPr><p:cNvPr id=\"5\" name=\"Accent Circle\"/>"
            L"<p:cNvSpPr/><p:nvPr/></p:nvSpPr>"
            L"<p:spPr><a:xfrm><a:off x=\"%s\" y=\"%s\"/><a:ext cx=\"%s\" cy=\"%s\"/></a:xfrm>"
            L"<a:prstGeom "
            L"prst=\"ellipse\"><a:avLst/></a:prstGeom>%s<a:ln><a:noFill/></a:ln></p:spPr>"
            L"</p:sp>",
            wxString{ std::to_wstring(circleX) }, wxString{ std::to_wstring(circleY) },
            wxString{ std::to_wstring(circleSize) }, wxString{ std::to_wstring(circleSize) },
            solidFillXml(accentColor, 16));
        }

    // solid accent bar running the full height of the left edge
    wxString accentBarXml;
    if (isThemed)
        {
        const long long barWidth{ std::llround(static_cast<double>(slideCx) * 0.016) };
        accentBarXml = wxString::Format(
            L"<p:sp><p:nvSpPr><p:cNvPr id=\"6\" name=\"Accent Bar\"/>"
            L"<p:cNvSpPr/><p:nvPr/></p:nvSpPr>"
            L"<p:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"%s\" cy=\"%s\"/></a:xfrm>"
            L"<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>%s<a:ln><a:noFill/></a:ln></p:spPr>"
            L"</p:sp>",
            wxString{ std::to_wstring(barWidth) }, wxString{ std::to_wstring(slideCy) },
            solidFillXml(accentColor, 100));
        }

    return wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        L"<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        L"xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        L"xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
        L"<p:cSld>%s<p:spTree>"
        L"<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        L"<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
        L"<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
        L"%s%s%s%s%s%s"
        L"</p:spTree></p:cSld>"
        L"<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>"
        L"%s"
        L"</p:sld>",
        bgXml, accentCircleXml, accentBarXml, titleBox, titleRuleXml, subtitleBox, publisherBox,
        transitionXml);
    }

//------------------------------------------------------
Wisteria::ReportPowerPointExport::ReportPowerPointExport(const std::vector<Canvas*>& canvases,
                                                         const wxString& filePath,
                                                         const PowerPointExportOptions& options)
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

    const wxSize slideEMU{ options.GetSlideSizeEMU() };
    const wxSize slideDIPs{ options.GetSlideSizeDIPs() };
    const long long slideCx{ slideEMU.GetWidth() };
    const long long slideCy{ slideEMU.GetHeight() };

    // render every page and collect its notes
    std::vector<RenderedPage> rendered;
    rendered.reserve(pages.size());
    bool anyNotes{ false };
    for (auto* canvas : pages)
        {
        RenderedPage page;
        if (options.m_includeAccessibilityNotes)
            {
            page.m_notes = CollectAccessibilityText(canvas);
            if (!page.m_notes.empty())
                {
                anyNotes = true;
                }
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
        RenderCanvas(canvas, renderSize, page.m_svg, page.m_png);
        rendered.push_back(std::move(page));
        }

    const bool includeTitleSlide{ options.m_includeTitleSlide && !options.m_title.empty() };

    // Presentation relationship IDs are rId1 for the master, rId2 for presProps,
    // rId3 for the optional notes master, rId9 for the optional title slide,
    // and rId10 onward for the report pages.
    const wxString notesMasterRelId{ L"rId3" };
    const wxString titleSlideRelId{ L"rId9" };
    constexpr size_t SLIDE_REL_START{ 10 };

    // build the variable parts
    wxString contentTypes{
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        L"<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        L"<Default Extension=\"rels\" "
        L"ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        L"<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        L"<Default Extension=\"png\" ContentType=\"image/png\"/>"
        L"<Default Extension=\"svg\" ContentType=\"image/svg+xml\"/>"
        L"<Override PartName=\"/ppt/presentation.xml\" "
        L"ContentType=\"application/vnd.openxmlformats-officedocument."
        L"presentationml.presentation.main+xml\"/>"
        L"<Override PartName=\"/ppt/presProps.xml\" "
        L"ContentType=\"application/"
        L"vnd.openxmlformats-officedocument.presentationml.presProps+xml\"/>"
        L"<Override PartName=\"/ppt/theme/theme1.xml\" "
        L"ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
        L"<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" "
        L"ContentType=\"application/vnd.openxmlformats-officedocument."
        L"presentationml.slideMaster+xml\"/>"
        L"<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" "
        L"ContentType=\"application/vnd.openxmlformats-officedocument."
        L"presentationml.slideLayout+xml\"/>"
        L"<Override PartName=\"/docProps/core.xml\" "
        L"ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
        L"<Override PartName=\"/docProps/app.xml\" "
        L"ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
    };
    if (includeTitleSlide)
        {
        contentTypes += L"<Override PartName=\"/ppt/slides/slide0.xml\" "
                        L"ContentType=\"application/"
                        L"vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
        }
    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        contentTypes +=
            wxString::Format(L"<Override PartName=\"/ppt/slides/slide%zu.xml\" "
                             L"ContentType=\"application/"
                             L"vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>",
                             pageIndex + 1);
        }
    if (anyNotes)
        {
        contentTypes +=
            L"<Override PartName=\"/ppt/notesMasters/notesMaster1.xml\" "
            L"ContentType=\"application/vnd.openxmlformats-officedocument."
            L"presentationml.notesMaster+xml\"/>"
            L"<Override PartName=\"/ppt/theme/theme2.xml\" "
            L"ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>";
        for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
            {
            if (!rendered[pageIndex].m_notes.empty())
                {
                contentTypes +=
                    wxString::Format(L"<Override PartName=\"/ppt/notesSlides/notesSlide%zu.xml\" "
                                     L"ContentType=\"application/vnd.openxmlformats-officedocument."
                                     L"presentationml.notesSlide+xml\"/>",
                                     pageIndex + 1);
                }
            }
        }
    contentTypes += L"</Types>";

    const wxString rootRels{ BuildRelationshipsXml(
        { { L"rId1", wxString{ REL_OFFICE_DOCUMENT }, L"ppt/presentation.xml" },
          { L"rId2", wxString{ REL_CORE_PROPS }, L"docProps/core.xml" },
          { L"rId3", wxString{ REL_EXTENDED_PROPS }, L"docProps/app.xml" } }) };

    const wxString nowUtc{ wxDateTime::Now().ToUTC().Format(L"%Y-%m-%dT%H:%M:%SZ") };
    const wxString coreProps{ wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        L"<cp:coreProperties "
        L"xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        L"xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        L"xmlns:dcterms=\"http://purl.org/dc/terms/\" "
        L"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
        L"<dc:title>%s</dc:title><dc:subject>%s</dc:subject><dc:creator>%s</dc:creator>"
        L"<cp:keywords>%s</cp:keywords><cp:lastModifiedBy>%s</cp:lastModifiedBy>"
        L"<dcterms:created xsi:type=\"dcterms:W3CDTF\">%s</dcterms:created>"
        L"<dcterms:modified xsi:type=\"dcterms:W3CDTF\">%s</dcterms:modified>"
        L"</cp:coreProperties>",
        EscapeXml(options.m_title), EscapeXml(options.m_subject), EscapeXml(options.m_author),
        EscapeXml(options.m_keywords), EscapeXml(options.m_author), nowUtc, nowUtc) };

    const wxString appName{ (wxTheApp != nullptr) ? wxTheApp->GetAppDisplayName() : wxString{} };
    const wxString companyElement{ options.m_publisher.empty() ?
                                       wxString{} :
                                       wxString::Format(L"<Company>%s</Company>",
                                                        EscapeXml(options.m_publisher)) };
    const wxString appProps{ wxString::Format(
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        L"<Properties "
        L"xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" "
        L"xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\">"
        L"<Application>%s</Application><Slides>%zu</Slides><ScaleCrop>false</ScaleCrop>"
        L"<LinksUpToDate>false</LinksUpToDate><SharedDoc>false</SharedDoc>"
        L"<HyperlinksChanged>false</HyperlinksChanged>%s<AppVersion>16.0000</AppVersion>"
        L"</Properties>",
        EscapeXml(appName), pages.size() + (includeTitleSlide ? 1 : 0), companyElement) };

    wxString presProps{
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        L"<p:presentationPr "
        L"xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        L"xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        L"relationships\" "
        L"xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
    };
    if (options.m_loopContinuously)
        {
        presProps += L"<p:showPr loop=\"1\" useTimings=\"1\"><p:present/><p:sldAll/></p:showPr>";
        }
    else if (options.m_advanceAutomatically)
        {
        presProps += L"<p:showPr useTimings=\"1\"><p:present/><p:sldAll/></p:showPr>";
        }
    presProps += L"</p:presentationPr>";

    // slide IDs must be >= 256 per the OOXML schema (ST_SlideId minInclusive)
    wxString sldIdLst;
    if (includeTitleSlide)
        {
        sldIdLst += wxString::Format(L"<p:sldId id=\"256\" r:id=\"%s\"/>", titleSlideRelId);
        }
    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        sldIdLst += wxString::Format(L"<p:sldId id=\"%zu\" r:id=\"rId%zu\"/>", 257 + pageIndex,
                                     SLIDE_REL_START + pageIndex);
        }

    wxString sldSz{ wxString::Format(L"<p:sldSz cx=\"%s\" cy=\"%s\"",
                                     wxString{ std::to_wstring(slideCx) },
                                     wxString{ std::to_wstring(slideCy) }) };
    if (options.m_slideSize == PowerPointExportOptions::SlideSize::Widescreen16x9)
        {
        sldSz += L" type=\"screen16x9\"";
        }
    else if (options.m_slideSize == PowerPointExportOptions::SlideSize::Standard4x3)
        {
        sldSz += L" type=\"screen4x3\"";
        }
    sldSz += L"/>";

    wxString presentation{
        L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        L"<p:presentation "
        L"xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        L"xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
        L"xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
        L"<p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>"
    };
    if (anyNotes)
        {
        presentation += wxString::Format(
            L"<p:notesMasterIdLst><p:notesMasterId r:id=\"%s\"/></p:notesMasterIdLst>",
            notesMasterRelId);
        }
    presentation += wxString::Format(L"<p:sldIdLst>%s</p:sldIdLst>", sldIdLst);
    presentation += sldSz;
    presentation += wxString::Format(L"<p:notesSz cx=\"%s\" cy=\"%s\"/>",
                                     wxString{ std::to_wstring(NOTES_PAGE_WIDTH_EMU) },
                                     wxString{ std::to_wstring(NOTES_PAGE_HEIGHT_EMU) });
    presentation += L"<p:defaultTextStyle/></p:presentation>";

    std::vector<std::tuple<wxString, wxString, wxString>> presRels{
        { L"rId1", wxString{ REL_SLIDE_MASTER }, L"slideMasters/slideMaster1.xml" },
        { L"rId2", wxString{ REL_PRES_PROPS }, L"presProps.xml" }
    };
    if (anyNotes)
        {
        presRels.emplace_back(notesMasterRelId, wxString{ REL_NOTES_MASTER },
                              L"notesMasters/notesMaster1.xml");
        }
    if (includeTitleSlide)
        {
        presRels.emplace_back(titleSlideRelId, wxString{ REL_SLIDE }, L"slides/slide0.xml");
        }
    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        presRels.emplace_back(wxString::Format(L"rId%zu", SLIDE_REL_START + pageIndex),
                              wxString{ REL_SLIDE },
                              wxString::Format(L"slides/slide%zu.xml", pageIndex + 1));
        }
    const wxString presRelsXml{ BuildRelationshipsXml(presRels) };

    const wxString masterRels{ BuildRelationshipsXml(
        { { L"rId1", wxString{ REL_SLIDE_LAYOUT }, L"../slideLayouts/slideLayout1.xml" },
          { L"rId2", wxString{ REL_THEME }, L"../theme/theme1.xml" } }) };
    const wxString layoutRels{ BuildRelationshipsXml(
        { { L"rId1", wxString{ REL_SLIDE_MASTER }, L"../slideMasters/slideMaster1.xml" } }) };
    // PowerPoint refuses to open a package where the notes master shares the slide master's theme
    const wxString notesMasterRels{ BuildRelationshipsXml(
        { { L"rId1", wxString{ REL_THEME }, L"../theme/theme2.xml" } }) };
    const wxString titleSlideRels{ BuildRelationshipsXml(
        { { L"rId1", wxString{ REL_SLIDE_LAYOUT }, L"../slideLayouts/slideLayout1.xml" } }) };
    const wxString transitionXml{ BuildTransitionXml(options) };
    const wxString titleSlideXml{ includeTitleSlide ?
                                      BuildTitleSlideXml(options, slideCx, slideCy, transitionXml) :
                                      wxString{} };

    // write the package
    wxFFileOutputStream fileStream{ filePath };
    if (!fileStream.IsOk())
        {
        wxMessageBox(wxString::Format(_(L"Failed to save PowerPoint report to \"%s\"."), filePath),
                     _(L"Export Error"), wxOK | wxICON_ERROR);
        return;
        }
    wxZipOutputStream zipStream{ fileStream };

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
    const auto addBinary = [&zipStream](const wxString& partPath, const void* data,
                                        const size_t length) -> bool
    {
        // PNG bytes are already compressed, so store them without extra deflation
        zipStream.SetLevel(0);
        if (!zipStream.PutNextEntry(new wxZipEntry{ partPath }))
            {
            return false;
            }
        if (length > 0)
            {
            zipStream.Write(data, length);
            }
        return zipStream.CloseEntry();
    };

    bool ok{ addText(L"[Content_Types].xml", contentTypes) && addText(L"_rels/.rels", rootRels) &&
             addText(L"docProps/core.xml", coreProps) && addText(L"docProps/app.xml", appProps) &&
             addText(L"ppt/presentation.xml", presentation) &&
             addText(L"ppt/_rels/presentation.xml.rels", presRelsXml) &&
             addText(L"ppt/presProps.xml", presProps) &&
             addText(L"ppt/theme/theme1.xml", wxString{ THEME_XML }) &&
             addText(L"ppt/slideMasters/slideMaster1.xml", wxString{ SLIDE_MASTER_XML }) &&
             addText(L"ppt/slideMasters/_rels/slideMaster1.xml.rels", masterRels) &&
             addText(L"ppt/slideLayouts/slideLayout1.xml", wxString{ SLIDE_LAYOUT_XML }) &&
             addText(L"ppt/slideLayouts/_rels/slideLayout1.xml.rels", layoutRels) };
    if (anyNotes)
        {
        ok = ok && addText(L"ppt/notesMasters/notesMaster1.xml", wxString{ NOTES_MASTER_XML }) &&
             addText(L"ppt/notesMasters/_rels/notesMaster1.xml.rels", notesMasterRels) &&
             addText(L"ppt/theme/theme2.xml", wxString{ THEME_XML });
        }
    if (includeTitleSlide)
        {
        ok = ok && addText(L"ppt/slides/slide0.xml", titleSlideXml) &&
             addText(L"ppt/slides/_rels/slide0.xml.rels", titleSlideRels);
        }

    for (size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex)
        {
        const size_t oneBased{ pageIndex + 1 };
        const RenderedPage& page{ rendered[pageIndex] };
        const bool slideHasNotes{ anyNotes && !page.m_notes.empty() };

        ok = addBinary(wxString::Format(L"ppt/media/image%zu.png", oneBased), page.m_png.GetData(),
                       page.m_png.GetDataLen()) &&
             ok;
        ok = addText(wxString::Format(L"ppt/media/image%zu.svg", oneBased), page.m_svg) && ok;

        std::vector<std::tuple<wxString, wxString, wxString>> slideRels{
            { L"rId1", wxString{ REL_SLIDE_LAYOUT }, L"../slideLayouts/slideLayout1.xml" },
            { L"rId2", wxString{ REL_IMAGE },
              wxString::Format(L"../media/image%zu.png", oneBased) },
            { L"rId3", wxString{ REL_IMAGE }, wxString::Format(L"../media/image%zu.svg", oneBased) }
        };
        if (slideHasNotes)
            {
            slideRels.emplace_back(L"rId4", wxString{ REL_NOTES_SLIDE },
                                   wxString::Format(L"../notesSlides/notesSlide%zu.xml", oneBased));
            }
        ok = addText(wxString::Format(L"ppt/slides/_rels/slide%zu.xml.rels", oneBased),
                     BuildRelationshipsXml(slideRels)) &&
             ok;

        const wxString altText{ page.m_notes.empty() ? pages[pageIndex]->GetLabel() :
                                                       page.m_notes.Left(MAX_ALT_TEXT_LENGTH) };
        // a short, distinct accessible name for the Selection Pane and screen readers,
        // separate from the (possibly long) alt text description above
        const wxString slideTitle{ !pages[pageIndex]->GetLabel().empty() ?
                                       pages[pageIndex]->GetLabel() :
                                       wxString::Format(_(L"Page %zu"), oneBased) };
        const wxString placement{ BuildPicturePlacementXml(slideCx, slideCy, page.m_pixelWidth,
                                                           page.m_pixelHeight) };
        const wxString slideXml{ wxString::Format(
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            L"<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
            L"xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
            L"xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
            L"<p:cSld><p:spTree>"
            L"<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
            L"<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
            L"<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
            L"<p:pic>"
            L"<p:nvPicPr>"
            L"<p:cNvPr id=\"2\" name=\"Page %zu\" title=\"%s\" descr=\"%s\"/>"
            L"<p:cNvPicPr><a:picLocks noChangeAspect=\"1\"/></p:cNvPicPr>"
            L"<p:nvPr/>"
            L"</p:nvPicPr>"
            L"<p:blipFill>"
            L"<a:blip r:embed=\"rId2\">"
            L"<a:extLst><a:ext uri=\"{96DAC541-7B7A-43D3-8B79-37D633B846F1}\">"
            L"<asvg:svgBlip "
            L"xmlns:asvg=\"http://schemas.microsoft.com/office/drawing/2016/SVG/main\" "
            L"r:embed=\"rId3\"/>"
            L"</a:ext></a:extLst>"
            L"</a:blip>"
            L"<a:stretch><a:fillRect/></a:stretch>"
            L"</p:blipFill>"
            L"<p:spPr>"
            L"<a:xfrm>%s</a:xfrm>"
            L"<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
            L"</p:spPr>"
            L"</p:pic>"
            L"</p:spTree></p:cSld>"
            L"<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>"
            L"%s"
            L"</p:sld>",
            oneBased, EscapeXmlAttribute(slideTitle), EscapeXmlAttribute(altText), placement,
            transitionXml) };
        ok = addText(wxString::Format(L"ppt/slides/slide%zu.xml", oneBased), slideXml) && ok;

        if (slideHasNotes)
            {
            wxString noteParagraphs;
            // notes text is free-form, so don't treat '\' as an escape character
            const wxArrayString noteLines = wxSplit(page.m_notes, L'\n', L'\0');
            for (const auto& noteLine : noteLines)
                {
                if (noteLine.empty())
                    {
                    noteParagraphs += L"<a:p><a:endParaRPr lang=\"en-US\"/></a:p>";
                    }
                else
                    {
                    noteParagraphs += wxString::Format(L"<a:p><a:r><a:t>%s</a:t></a:r></a:p>",
                                                       EscapeXml(noteLine));
                    }
                }
            const wxString notesSlideXml{ wxString::Format(
                L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                L"<p:notes xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
                L"xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
                L"xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
                L"<p:cSld><p:spTree>"
                L"<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
                L"<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
                L"<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
                L"<p:sp>"
                L"<p:nvSpPr><p:cNvPr id=\"2\" name=\"Notes Placeholder 2\"/>"
                L"<p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr>"
                L"<p:nvPr><p:ph type=\"body\" idx=\"1\"/></p:nvPr></p:nvSpPr>"
                L"<p:spPr/>"
                L"<p:txBody><a:bodyPr/><a:lstStyle/>%s</p:txBody>"
                L"</p:sp>"
                L"</p:spTree></p:cSld>"
                L"<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>"
                L"</p:notes>",
                noteParagraphs) };
            ok = addText(wxString::Format(L"ppt/notesSlides/notesSlide%zu.xml", oneBased),
                         notesSlideXml) &&
                 ok;
            ok =
                addText(wxString::Format(L"ppt/notesSlides/_rels/notesSlide%zu.xml.rels", oneBased),
                        BuildRelationshipsXml(
                            { { L"rId1", wxString{ REL_SLIDE },
                                wxString::Format(L"../slides/slide%zu.xml", oneBased) },
                              { L"rId2", wxString{ REL_NOTES_MASTER },
                                L"../notesMasters/notesMaster1.xml" } })) &&
                ok;
            }
        }

    const bool closedOk{ zipStream.Close() && fileStream.Close() };
    if (!ok || !closedOk)
        {
        wxMessageBox(wxString::Format(_(L"Failed to save PowerPoint report to \"%s\"."), filePath),
                     _(L"Export Error"), wxOK | wxICON_ERROR);
        }
    }
