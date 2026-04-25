///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriadoc.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wisteriadoc.h"
#include "wisteriaapp.h"
#include "wisteriaview.h"
#include <variant>

wxIMPLEMENT_DYNAMIC_CLASS(WisteriaDoc, wxDocument);

//-------------------------------------------
bool WisteriaDoc::DoSaveDocument(const wxString& filename)
    {
    try
        {
        SaveProject(filename);

        return true;
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Save Error"), wxOK | wxICON_ERROR);
        return false;
        }
    }

//-------------------------------------------
bool WisteriaDoc::OnNewDocument()
    {
    if (!wxDocument::OnNewDocument())
        {
        return false;
        }

    SetTitle(_(L"Untitled"));
    Modify(true);

    return true;
    }

//-------------------------------------------
bool WisteriaDoc::OnOpenDocument(const wxString& filename)
    {
    if (!wxDocument::OnOpenDocument(filename))
        {
        return false;
        }

    SetFilename(filename, true);
    SetTitle(wxFileName{ filename }.GetName());
    Modify(false);
    UpdateAllViews();
    return true;
    }

//-------------------------------------------
void WisteriaDoc::SaveProject(const wxString& filePath) const
    {
    auto* view = dynamic_cast<WisteriaView*>(GetFirstView());
    if (view == nullptr)
        {
        wxASSERT_MSG(view, L"Invalid view connected to document?!");
        return;
        }

    const wxFileName projectDir(filePath);

    // project name
    const wxString projectName = view->GetReportBuilder().GetName().empty() ?
                                     wxFileName{ filePath }.GetName() :
                                     view->GetReportBuilder().GetName();

    auto root = wxSimpleJSON::Create(
        wxString::Format(L"{\"name\": \"%s\", \"watermark\": {}, \"print\": {}, "
                         L"\"datasets\": [], \"constants\": [], \"pages\": []}",
                         EscapeJsonStr(projectName)),
        true);

    // watermark
    //----------
    const auto& wmLabel = view->GetReportBuilder().GetWatermarkLabel();
    const auto& wmColor = view->GetReportBuilder().GetWatermarkColor();
    if (!wmLabel.empty())
        {
        auto wmObj = root->GetProperty(L"watermark");
        wmObj->Add(L"label", wmLabel);
        if (wmColor.IsOk())
            {
            wmObj->Add(L"color", ColorToStr(wmColor));
            }
        }
    else
        {
        root->DeleteProperty(L"watermark");
        }

    // print settings
    //---------------
    const auto printOrientation = view->GetReportBuilder().GetPrintOrientation();
    const auto paperSize = view->GetReportBuilder().GetPaperSize();
    if (printOrientation != wxPrintOrientation::wxPORTRAIT ||
        paperSize != wxPaperSize::wxPAPER_NONE)
        {
        auto printObj = root->GetProperty(L"print");
        if (printOrientation == wxPrintOrientation::wxLANDSCAPE)
            {
            printObj->Add(L"orientation", wxString{ L"landscape" });
            }
        else
            {
            printObj->Add(L"orientation", wxString{ L"portrait" });
            }
        const auto paperStr = Wisteria::ReportEnumConvert::ConvertPaperSizeToString(paperSize);
        if (paperStr.has_value())
            {
            printObj->Add(L"paper-size", paperStr.value());
            }
        }
    else
        {
        root->DeleteProperty(L"print");
        }

    // datasets
    //---------
    const auto& datasets = view->GetReportBuilder().GetDatasets();
    const auto& importOpts = view->GetReportBuilder().GetDatasetImportOptions();
    const auto& transformOpts = view->GetReportBuilder().GetDatasetTransformOptions();

    auto datasetsArray = root->GetProperty(L"datasets");
    const auto& insertionOrder = view->GetReportBuilder().GetDatasetInsertionOrder();
    for (const auto& dsName : insertionOrder)
        {
        // only write top-level datasets (those with a file path);
        // derived datasets (subsets, pivots, merges) are nested under their parent
        const auto optIt = importOpts.find(dsName);
        if (optIt == importOpts.cend() || optIt->second.m_filePath.empty() ||
            !datasets.contains(dsName))
            {
            continue;
            }

        // build dataset template with name and path up front
        wxString dsTmpl = L"{";
            {
            wxFileName dataPath(optIt->second.m_filePath);
            // only write "name" if it differs from the file stem
            const auto fileStem = dataPath.GetName();
            if (dsName.CmpNoCase(fileStem) != 0)
                {
                dsTmpl += wxString::Format(L"\"name\": \"%s\", ", EscapeJsonStr(dsName));
                }
            dataPath.MakeRelativeTo(projectDir.GetPath());
            dsTmpl += wxString::Format(L"\"path\": \"%s\", ",
                                       EscapeJsonStr(dataPath.GetFullPath(wxPATH_UNIX)));
            }
        dsTmpl += L"\"categorical-columns\": [], \"date-columns\": [], "
                  L"\"recode-re\": [], \"columns-rename\": [], "
                  L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                  L"\"collapse-except\": [], \"formulas\": [], "
                  L"\"subsets\": [], \"pivots\": [], \"merges\": []}";
        auto dsNode = wxSimpleJSON::Create(dsTmpl);

        // look up transform options once; used both for import options (to reverse-map
        // post-rename column names to original file names) and for transform serialization
        const auto txIt = transformOpts.find(dsName);

            {
            const auto& opts = optIt->second;

            // importer override
            if (!opts.m_importer.empty())
                {
                dsNode->Add(L"importer", opts.m_importer);
                }

            // worksheet
            if (std::holds_alternative<wxString>(opts.m_worksheet))
                {
                const auto& ws = std::get<wxString>(opts.m_worksheet);
                if (!ws.empty())
                    {
                    dsNode->Add(L"worksheet", ws);
                    }
                }
            else
                {
                const auto wsIdx = std::get<size_t>(opts.m_worksheet);
                if (wsIdx != 1)
                    {
                    dsNode->Add(L"worksheet", static_cast<double>(wsIdx));
                    }
                }

            SaveDatasetImportOptions(
                dsNode, opts.m_columnPreviewInfo, opts.m_importInfo,
                (txIt != transformOpts.cend()) ?
                    txIt->second.m_columnRenames :
                    std::vector<Wisteria::ReportBuilder::DatasetColumnRename>{});
            }

        // transform options and formulas
        if (txIt != transformOpts.cend())
            {
            SaveTransformOptions(dsNode, txIt->second);
            SaveFormulas(dsNode, txIt->second.m_formulas);
            }

        // nested pivots, subsets, and merges
        SavePivots(dsNode, dsName);
        SaveSubsets(dsNode, dsName);
        SaveMerges(dsNode, dsName);

        // clean up empty arrays
        for (const auto& key :
             { L"categorical-columns", L"date-columns", L"recode-re", L"columns-rename",
               L"mutate-categorical-columns", L"collapse-min", L"collapse-except", L"formulas",
               L"subsets", L"pivots", L"merges" })
            {
            if (dsNode->GetProperty(key)->ArraySize() == 0)
                {
                dsNode->DeleteProperty(key);
                }
            }

        datasetsArray->ArrayAdd(dsNode);
        }

    // constants
    //----------
    const auto& constants = view->GetReportBuilder().GetConstants();
    if (!constants.empty())
        {
        auto constArray = root->GetProperty(L"constants");
        for (const auto& c : constants)
            {
            auto cObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            cObj->Add(L"name", c.m_name);
            double dVal{};
            if (c.m_value.ToDouble(&dVal))
                {
                cObj->Add(L"value", dVal);
                }
            else
                {
                cObj->Add(L"value", c.m_value);
                }
            constArray->ArrayAdd(cObj);
            }
        }
    else
        {
        root->DeleteProperty(L"constants");
        }

    // pages
    //------
    auto pagesArray = root->GetProperty(L"pages");
    for (const auto* canvas : view->GetPages())
        {
        if (canvas == nullptr)
            {
            continue;
            }

        const auto& nameTmpl = canvas->GetNameTemplate();
        const auto& pageName = nameTmpl.empty() ? canvas->GetLabel() : nameTmpl;
        auto pageObj = pageName.empty() ?
                           wxSimpleJSON::Create(L"{\"rows\": []}") :
                           wxSimpleJSON::Create(L"{\"name\": \"" + EscapeJsonStr(pageName) +
                                                L"\", \"rows\": []}");

        auto rowsArray = pageObj->GetProperty(L"rows");
        const auto [rowCount, colCount] = canvas->GetFixedObjectsGridSize();
        for (size_t row = 0; row < rowCount; ++row)
            {
            std::vector<wxSimpleJSON::Ptr_t> itemNodes;
            for (size_t col = 0; col < colCount; ++col)
                {
                const auto item = canvas->GetFixedObject(row, col);
                // skip legend labels (serialized as part of the graph)
                if (item != nullptr && item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
                    {
                    const auto* label =
                        dynamic_cast<const Wisteria::GraphItems::Label*>(item.get());
                    if (label != nullptr && label->IsLegend())
                        {
                        continue;
                        }
                    }
                auto itemNode = SavePageItem(item.get(), canvas);
                if (itemNode != nullptr)
                    {
                    itemNodes.push_back(itemNode);
                    }
                }
            // skip rows where all items were filtered out (e.g., legend-only rows)
            if (!itemNodes.empty())
                {
                auto rowObj = wxSimpleJSON::Create(L"{\"items\": []}");
                auto itemsArray = rowObj->GetProperty(L"items");
                for (auto& node : itemNodes)
                    {
                    itemsArray->ArrayAdd(node);
                    }
                rowsArray->ArrayAdd(rowObj);
                }
            }

        pagesArray->ArrayAdd(pageObj);
        }

    wxString output = root->Print();
    output.Replace(L"\t", L" ");
    wxFile outFile(filePath, wxFile::write);
    if (outFile.IsOpened())
        {
        const auto utf8 = output.utf8_string();
        outFile.Write(utf8.c_str(), utf8.length());
        }
    }

//-------------------------------------------
wxString WisteriaDoc::EscapeJsonStr(const wxString& str)
    {
    wxString escaped = str;
    escaped.Replace(L"\\", L"\\\\");
    escaped.Replace(L"\"", L"\\\"");
    return escaped;
    }

//-------------------------------------------
wxString WisteriaDoc::ColorToStr(const wxColour& color) const
    {
    auto* view = dynamic_cast<WisteriaView*>(GetFirstView());
    if (view == nullptr)
        {
        wxASSERT_MSG(view, L"Invalid view connected to document?!");
        return {};
        }

    auto hexStr = color.GetAsString(wxC2S_HTML_SYNTAX);
    // check constants table first (preserves {{ConstantName}} syntax)
    for (const auto& constant : view->GetReportBuilder().GetConstants())
        {
        if (constant.m_value.CmpNoCase(hexStr) == 0)
            {
            return L"{{" + constant.m_name + L"}}";
            }
        }
    // check named Wisteria colors
    for (const auto& [name, colorEnum] : Wisteria::ReportBuilder::GetColorMap())
        {
        if (Wisteria::Colors::ColorBrewer::GetColor(colorEnum) == color)
            {
            return wxString(name);
            }
        }
    return hexStr;
    }

//-------------------------------------------
wxString WisteriaDoc::SavePenToStr(const wxPen& pen) const
    {
    if (!pen.IsOk() || pen == wxNullPen)
        {
        return L"null";
        }

    const auto colorStr = ColorToStr(pen.GetColour());
    const auto styleStr = Wisteria::ReportEnumConvert::ConvertPenStyleToString(pen.GetStyle());

    const bool isDefaultWidth = (pen.GetWidth() <= 1);
    const bool isDefaultStyle = (!styleStr.has_value() || styleStr.value() == L"solid");

    // if only a color with default width and style, just return the color string
    if (isDefaultWidth && isDefaultStyle)
        {
        return wxString::Format(L"\"%s\"", colorStr);
        }

    wxString result = L"{";
    result += wxString::Format(L"\"color\": \"%s\"", colorStr);
    if (!isDefaultWidth)
        {
        result += wxString::Format(L", \"width\": %d", pen.GetWidth());
        }
    if (!isDefaultStyle)
        {
        result += wxString::Format(L", \"style\": \"%s\"", styleStr.value());
        }
    result += L"}";
    return result;
    }

//-------------------------------------------
wxString WisteriaDoc::SaveBrushToStr(const wxBrush& brush) const
    {
    if (!brush.IsOk() || brush == wxNullBrush)
        {
        return L"null";
        }

    const auto colorStr = ColorToStr(brush.GetColour());
    const auto styleStr = Wisteria::ReportEnumConvert::ConvertBrushStyleToString(brush.GetStyle());

    // if solid style, just return the color string
    if (!styleStr.has_value() || styleStr.value() == L"solid")
        {
        return wxString::Format(L"\"%s\"", colorStr);
        }

    return wxString::Format(L"{\"color\": \"%s\", \"style\": \"%s\"}", colorStr, styleStr.value());
    }

//-------------------------------------------
void WisteriaDoc::SaveItem(wxSimpleJSON::Ptr_t& itemNode,
                           const Wisteria::GraphItems::GraphItemBase* item,
                           const Wisteria::Canvas* canvas)
    {
    if (item == nullptr || canvas == nullptr)
        {
        return;
        }

    // ID
    if (item->GetId() != wxID_ANY)
        {
        itemNode->Add(L"id", static_cast<double>(item->GetId()));
        }

    // scaling — use the original cached value, not the canvas-overwritten one
    const double itemScaling = item->GetGraphItemInfo().GetOriginalCanvasScaling();
    if (!compare_doubles(itemScaling, 1.0))
        {
        itemNode->Add(L"scaling", itemScaling);
        }

    // canvas-margins [top, right, bottom, left]
    const auto cmt = item->GetTopCanvasMargin();
    const auto cmr = item->GetRightCanvasMargin();
    const auto cmb = item->GetBottomCanvasMargin();
    const auto cml = item->GetLeftCanvasMargin();
    if (cmt != 0 || cmr != 0 || cmb != 0 || cml != 0)
        {
        auto cmArray = itemNode->GetProperty(L"canvas-margins");
        cmArray->ArrayAdd(static_cast<double>(cmt));
        cmArray->ArrayAdd(static_cast<double>(cmr));
        cmArray->ArrayAdd(static_cast<double>(cmb));
        cmArray->ArrayAdd(static_cast<double>(cml));
        }
    else
        {
        itemNode->DeleteProperty(L"canvas-margins");
        }

    // padding [top, right, bottom, left]
    const auto pt = item->GetTopPadding();
    const auto pr = item->GetRightPadding();
    const auto pb = item->GetBottomPadding();
    const auto pl = item->GetLeftPadding();
    if (pt != 0 || pr != 0 || pb != 0 || pl != 0)
        {
        auto padArray = itemNode->GetProperty(L"padding");
        padArray->ArrayAdd(static_cast<double>(pt));
        padArray->ArrayAdd(static_cast<double>(pr));
        padArray->ArrayAdd(static_cast<double>(pb));
        padArray->ArrayAdd(static_cast<double>(pl));
        }
    else
        {
        itemNode->DeleteProperty(L"padding");
        }

    // outline [top, right, bottom, left]
    const auto& info = item->GetGraphItemInfo();
    const bool oTop = info.IsShowingTopOutline();
    const bool oRight = info.IsShowingRightOutline();
    const bool oBottom = info.IsShowingBottomOutline();
    const bool oLeft = info.IsShowingLeftOutline();
    if (oTop || oRight || oBottom || oLeft)
        {
        auto olArray = itemNode->GetProperty(L"outline");
        olArray->ArrayAdd(oTop);
        olArray->ArrayAdd(oRight);
        olArray->ArrayAdd(oBottom);
        olArray->ArrayAdd(oLeft);
        }
    else
        {
        auto olArray = itemNode->GetProperty(L"outline");
        olArray->ArrayAdd(false);
        olArray->ArrayAdd(false);
        olArray->ArrayAdd(false);
        olArray->ArrayAdd(false);
        }

    // show (default is true)
    if (!item->IsShown())
        {
        itemNode->Add(L"show", false);
        }

    // anchoring (default is Center)
    const auto anchoringStr =
        Wisteria::ReportEnumConvert::ConvertAnchoringToString(item->GetAnchoring());
    if (anchoringStr.has_value() && item->GetAnchoring() != Wisteria::Anchoring::Center)
        {
        itemNode->Add(L"anchoring", anchoringStr.value());
        }

    // horizontal-page-alignment (default is LeftAligned)
    const auto hpaStr = Wisteria::ReportEnumConvert::ConvertPageHorizontalAlignmentToString(
        item->GetPageHorizontalAlignment());
    if (hpaStr.has_value() &&
        item->GetPageHorizontalAlignment() != Wisteria::PageHorizontalAlignment::LeftAligned)
        {
        itemNode->Add(L"horizontal-page-alignment", hpaStr.value());
        }

    // vertical-page-alignment (default is TopAligned)
    const auto vpaStr = Wisteria::ReportEnumConvert::ConvertPageVerticalAlignmentToString(
        item->GetPageVerticalAlignment());
    if (vpaStr.has_value() &&
        item->GetPageVerticalAlignment() != Wisteria::PageVerticalAlignment::TopAligned)
        {
        itemNode->Add(L"vertical-page-alignment", vpaStr.value());
        }

    // relative-alignment (default is Centered)
    const auto raStr =
        Wisteria::ReportEnumConvert::ConvertRelativeAlignmentToString(item->GetRelativeAlignment());
    if (raStr.has_value() && item->GetRelativeAlignment() != Wisteria::RelativeAlignment::Centered)
        {
        itemNode->Add(L"relative-alignment", raStr.value());
        }

    // fixed-width (default is false)
    if (item->IsFixedWidthOnCanvas())
        {
        itemNode->Add(L"fixed-width", true);
        }

    // fit-row-to-content (default is false)
    if (item->IsFittingCanvasRowHeightToContent())
        {
        itemNode->Add(L"fit-row-to-content", true);
        }
    }

//-------------------------------------------
wxString WisteriaDoc::SaveLabelPropertiesToStr(const Wisteria::GraphItems::Label& label) const
    {
    wxString json = L"{";

    // text (prefer template if it has {{constants}})
    const auto textTmpl = label.GetPropertyTemplate(L"text");
    const auto& text = textTmpl.empty() ? label.GetText() : textTmpl;
    if (!text.empty())
        {
        json += L"\"text\": \"" + EscapeJsonStr(text) + L"\"";
        }

    // bold
    if (label.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"bold\": true";
        }

    // color (font color, if not default black)
    const auto& fontColor = label.GetFontColor();
    if (fontColor.IsOk() && fontColor != *wxBLACK)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        const auto colorTmpl = label.GetPropertyTemplate(L"color");
        json += L"\"color\": \"" +
                EscapeJsonStr(colorTmpl.empty() ? ColorToStr(fontColor) : colorTmpl) + L"\"";
        }

    // background color
    const auto& bgColor = label.GetFontBackgroundColor();
    if (bgColor.IsOk() && bgColor != wxTransparentColour)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"background\": \"" + ColorToStr(bgColor) + L"\"";
        }

    // orientation (default is horizontal)
    if (label.GetTextOrientation() == Wisteria::Orientation::Vertical)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"orientation\": \"vertical\"";
        }

    // line-spacing (default is 1)
    if (!compare_doubles(label.GetLineSpacing(), 1.0))
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += wxString::Format(L"\"line-spacing\": %g", label.GetLineSpacing());
        }

    // text-alignment (default is flush-left)
    if (label.GetTextAlignment() != Wisteria::TextAlignment::FlushLeft)
        {
        const auto taStr =
            Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(label.GetTextAlignment());
        if (taStr.has_value())
            {
            if (json.Last() != L'{')
                {
                json += L", ";
                }
            json += L"\"text-alignment\": \"" + taStr.value() + L"\"";
            }
        }

    // padding
    if (label.GetTopPadding() != 0 || label.GetRightPadding() != 0 ||
        label.GetBottomPadding() != 0 || label.GetLeftPadding() != 0)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += wxString::Format(L"\"padding\": [%d, %d, %d, %d]", label.GetTopPadding(),
                                 label.GetRightPadding(), label.GetBottomPadding(),
                                 label.GetLeftPadding());
        }

    // pen
    const auto& pen = label.GetPen();
    if (pen.IsOk() && pen != wxNullPen)
        {
        if (json.Last() != L'{')
            {
            json += L", ";
            }
        json += L"\"pen\": " + SavePenToStr(pen);
        }

    // header
    const auto& header = label.GetHeaderInfo();
    if (header.IsEnabled())
        {
        wxString hdrStr = L"{";
        if (header.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
            {
            hdrStr += L"\"bold\": true";
            }
        if (header.GetFontColor().IsOk() && header.GetFontColor() != *wxBLACK)
            {
            if (hdrStr.Last() != L'{')
                {
                hdrStr += L", ";
                }
            hdrStr += L"\"color\": \"" + ColorToStr(header.GetFontColor()) + L"\"";
            }
        if (!compare_doubles(header.GetRelativeScaling(), 1.0))
            {
            if (hdrStr.Last() != L'{')
                {
                hdrStr += L", ";
                }
            hdrStr += wxString::Format(L"\"relative-scaling\": %g", header.GetRelativeScaling());
            }
        if (header.GetLabelAlignment() != Wisteria::TextAlignment::FlushLeft)
            {
            const auto htaStr = Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(
                header.GetLabelAlignment());
            if (htaStr.has_value())
                {
                if (hdrStr.Last() != L'{')
                    {
                    hdrStr += L", ";
                    }
                hdrStr += L"\"text-alignment\": \"" + htaStr.value() + L"\"";
                }
            }
        hdrStr += L"}";
        if (hdrStr != L"{}")
            {
            if (json.Last() != L'{')
                {
                json += L", ";
                }
            json += L"\"header\": " + hdrStr;
            }
        }

    json += L"}";
    return json;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SaveLabel(const Wisteria::GraphItems::Label* label,
                                           const Wisteria::Canvas* canvas) const
    {
    if (label == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    // empty-spacer: not shown, canvas height proportion is 0
    if (!label->IsShown() && label->GetCanvasHeightProportion().has_value() &&
        compare_doubles(label->GetCanvasHeightProportion().value(), 0.0))
        {
        return wxSimpleJSON::Create(L"{\"type\": \"empty-spacer\"}");
        }

    // spacer: not shown (but not an empty-spacer)
    if (!label->IsShown())
        {
        return wxSimpleJSON::Create(L"{\"type\": \"spacer\"}");
        }

    // build header JSON string first (needed for template)
    wxString headerStr;
    const auto& header = label->GetHeaderInfo();
    if (header.IsEnabled())
        {
        wxString hdrObj = L"{";
        if (header.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
            {
            hdrObj += L"\"bold\": true";
            }
        if (header.GetFontColor().IsOk() && header.GetFontColor() != *wxBLACK)
            {
            if (hdrObj.Last() != L'{')
                {
                hdrObj += L", ";
                }
            hdrObj += L"\"color\": \"" + EscapeJsonStr(ColorToStr(header.GetFontColor())) + L"\"";
            }
        if (!compare_doubles(header.GetRelativeScaling(), 1.0))
            {
            if (hdrObj.Last() != L'{')
                {
                hdrObj += L", ";
                }
            hdrObj += wxString::Format(L"\"relative-scaling\": %g", header.GetRelativeScaling());
            }
        if (header.GetLabelAlignment() != Wisteria::TextAlignment::FlushLeft)
            {
            const auto htaStr = Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(
                header.GetLabelAlignment());
            if (htaStr.has_value())
                {
                if (hdrObj.Last() != L'{')
                    {
                    hdrObj += L", ";
                    }
                hdrObj += L"\"text-alignment\": \"" + htaStr.value() + L"\"";
                }
            }
        hdrObj += L"}";
        if (hdrObj != L"{}")
            {
            headerStr = hdrObj;
            }
        }

    // build template with all sub-objects embedded
    wxString tmpl = L"{\"type\": \"label\"";
    if (!headerStr.empty())
        {
        tmpl += L", \"header\": " + headerStr;
        }
    const auto& pen = label->GetPen();
    if (pen.IsOk() && pen != wxNullPen)
        {
        tmpl += L", \"pen\": " + SavePenToStr(pen);
        }

    // left-image
    const auto leftImgPath = label->GetPropertyTemplate(L"left-image.path");
    if (!leftImgPath.empty())
        {
        tmpl += L", \"left-image\": {\"image-import\": {\"path\": \"" + EscapeJsonStr(leftImgPath) +
                L"\"}}";
        }

    // top-image
    const auto topImgPath = label->GetPropertyTemplate(L"top-image.path");
    if (!topImgPath.empty())
        {
        tmpl += L", \"top-image\": {\"image-import\": {\"path\": \"" + EscapeJsonStr(topImgPath) +
                L"\"}";
        if (label->GetTopImageOffset() != 0)
            {
            tmpl += wxString::Format(L", \"offset\": %zu", label->GetTopImageOffset());
            }
        tmpl += L"}";
        }

    // top-shape
    const auto& topShape = label->GetTopShape();
    if (topShape.has_value() && !topShape->empty())
        {
        if (topShape->size() == 1)
            {
            tmpl += L", \"top-shape\": ";
            }
        else
            {
            tmpl += L", \"top-shape\": [";
            }
        for (size_t i = 0; i < topShape->size(); ++i)
            {
            if (i > 0)
                {
                tmpl += L", ";
                }
            const auto& si = topShape->at(i);
            const auto siIconStr = Wisteria::ReportEnumConvert::ConvertIconToString(si.GetShape());
            tmpl += L"{";
            if (siIconStr.has_value())
                {
                tmpl += L"\"icon\": \"" + siIconStr.value() + L"\"";
                }
            const auto siSz = si.GetSizeDIPs();
            tmpl += wxString::Format(L", \"size\": {\"width\": %d, \"height\": %d}",
                                     siSz.GetWidth(), siSz.GetHeight());
            const auto& siPen = si.GetPen();
            if (siPen.IsOk() && siPen != wxNullPen)
                {
                tmpl += L", \"pen\": " + SavePenToStr(siPen);
                }
            const auto& siBrush = si.GetBrush();
            if (siBrush.IsOk() && siBrush != wxNullBrush)
                {
                tmpl += L", \"brush\": " + SaveBrushToStr(siBrush);
                }
            if (!compare_doubles(si.GetFillPercent(), math_constants::full))
                {
                tmpl += wxString::Format(L", \"fill-percent\": %g", si.GetFillPercent());
                }
            if (!si.GetText().empty())
                {
                tmpl += L", \"label\": \"" + EscapeJsonStr(si.GetText()) + L"\"";
                }
            tmpl += L"}";
            }
        if (topShape->size() > 1)
            {
            tmpl += L"]";
            }
        if (label->GetTopImageOffset() != 0 && topImgPath.empty())
            {
            tmpl += wxString::Format(L", \"top-shape-offset\": %zu", label->GetTopImageOffset());
            }
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    // text (prefer template if it has {{constants}})
    const auto textTemplate = label->GetPropertyTemplate(L"text");
    const auto& text = textTemplate.empty() ? label->GetText() : textTemplate;
    if (!text.empty())
        {
        node->Add(L"text", text);
        }

    // color (font color, if not default black)
    const auto& fontColor = label->GetFontColor();
    if (fontColor.IsOk() && fontColor != *wxBLACK)
        {
        const auto colorTemplate = label->GetPropertyTemplate(L"color");
        node->Add(L"color", colorTemplate.empty() ? ColorToStr(fontColor) : colorTemplate);
        }

    // background color
    const auto& bgColor = label->GetFontBackgroundColor();
    if (bgColor.IsOk() && bgColor != wxTransparentColour)
        {
        node->Add(L"background", ColorToStr(bgColor));
        }

    // bold
    if (label->GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
        {
        node->Add(L"bold", true);
        }

    // orientation (default is horizontal)
    if (label->GetTextOrientation() == Wisteria::Orientation::Vertical)
        {
        node->Add(L"orientation", wxString{ L"vertical" });
        }

    // visual style (default is no-label-style)
    if (label->GetLabelStyle() != Wisteria::LabelStyle::NoLabelStyle)
        {
        const auto styleStr =
            Wisteria::ReportEnumConvert::ConvertLabelStyleToString(label->GetLabelStyle());
        if (styleStr.has_value())
            {
            node->Add(L"style", styleStr.value());
            }
        }

    // line-spacing (default is 1)
    if (!compare_doubles(label->GetLineSpacing(), 1.0))
        {
        node->Add(L"line-spacing", label->GetLineSpacing());
        }

    // text-alignment (default is flush-left)
    if (label->GetTextAlignment() != Wisteria::TextAlignment::FlushLeft)
        {
        const auto taStr =
            Wisteria::ReportEnumConvert::ConvertTextAlignmentToString(label->GetTextAlignment());
        if (taStr.has_value())
            {
            node->Add(L"text-alignment", taStr.value());
            }
        }

    SaveItem(node, label, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SaveImage(const Wisteria::GraphItems::Image* image,
                                           const Wisteria::Canvas* canvas) const
    {
    if (image == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    wxString tmpl = L"{\"type\": \"image\"";

    // image-import from property templates
    const auto pathsTemplate = image->GetPropertyTemplate(L"image-import.paths");
    const auto pathTemplate = image->GetPropertyTemplate(L"image-import.path");
    const auto effectTemplate = image->GetPropertyTemplate(L"image-import.effect");
    const auto stitchTemplate = image->GetPropertyTemplate(L"image-import.stitch");

    if (!pathsTemplate.empty())
        {
        // multiple paths (tab-separated)
        tmpl += L", \"image-import\": {\"paths\": [";
        wxStringTokenizer tokenizer(pathsTemplate, L"\t");
        bool first = true;
        while (tokenizer.HasMoreTokens())
            {
            if (!first)
                {
                tmpl += L", ";
                }
            tmpl += L"\"" + EscapeJsonStr(MakeRelativePath(tokenizer.GetNextToken())) + L"\"";
            first = false;
            }
        tmpl += L"]";
        if (!stitchTemplate.empty())
            {
            tmpl += L", \"stitch\": \"" + EscapeJsonStr(stitchTemplate) + L"\"";
            }
        if (!effectTemplate.empty())
            {
            tmpl += L", \"effect\": \"" + EscapeJsonStr(effectTemplate) + L"\"";
            }
        tmpl += L"}";
        }
    else if (!pathTemplate.empty())
        {
        // single path
        tmpl += L", \"image-import\": {\"path\": \"" +
                EscapeJsonStr(MakeRelativePath(pathTemplate)) + L"\"";
        if (!effectTemplate.empty())
            {
            tmpl += L", \"effect\": \"" + EscapeJsonStr(effectTemplate) + L"\"";
            }
        tmpl += L"}";
        }

    // size (from cached original values)
    const auto widthStr = image->GetPropertyTemplate(L"size.width");
    const auto heightStr = image->GetPropertyTemplate(L"size.height");
    if (!widthStr.empty() || !heightStr.empty())
        {
        tmpl += L", \"size\": {";
        bool needComma = false;
        if (!widthStr.empty())
            {
            tmpl += L"\"width\": " + widthStr;
            needComma = true;
            }
        if (!heightStr.empty())
            {
            if (needComma)
                {
                tmpl += L", ";
                }
            tmpl += L"\"height\": " + heightStr;
            }
        tmpl += L"}";
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";
    auto node = wxSimpleJSON::Create(tmpl);

    // resize-method (default is DownscaleOrUpscale)
    if (image->GetResizeMethod() != Wisteria::ResizeMethod::DownscaleOrUpscale)
        {
        const auto rmStr =
            Wisteria::ReportEnumConvert::ConvertResizeMethodToString(image->GetResizeMethod());
        if (rmStr.has_value())
            {
            node->Add(L"resize-method", rmStr.value());
            }
        }

    SaveItem(node, image, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SaveShape(const Wisteria::GraphItems::Shape* shape,
                                           const Wisteria::Canvas* canvas) const
    {
    if (shape == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    const auto iconStr = Wisteria::ReportEnumConvert::ConvertIconToString(shape->GetShape());

    wxString tmpl = L"{\"type\": \"shape\"";
    if (iconStr.has_value())
        {
        tmpl += L", \"icon\": \"" + iconStr.value() + L"\"";
        }

    // size (prefer cached original values, fall back to current)
    const auto widthStr = shape->GetPropertyTemplate(L"size.width");
    const auto heightStr = shape->GetPropertyTemplate(L"size.height");
    if (!widthStr.empty() || !heightStr.empty())
        {
        tmpl += L", \"size\": {";
        bool needComma = false;
        if (!widthStr.empty())
            {
            tmpl += L"\"width\": " + widthStr;
            needComma = true;
            }
        if (!heightStr.empty())
            {
            if (needComma)
                {
                tmpl += L", ";
                }
            tmpl += L"\"height\": " + heightStr;
            }
        tmpl += L"}";
        }
    else
        {
        const auto sz = shape->GetSizeDIPS();
        tmpl += wxString::Format(L", \"size\": {\"width\": %d, \"height\": %d}", sz.GetWidth(),
                                 sz.GetHeight());
        }

    // pen (skip if default black solid width-1)
    const auto& pen = shape->GetPen();
    if (pen.IsOk() && pen != wxNullPen &&
        !(pen.GetColour() == *wxBLACK && pen.GetWidth() <= 1 && pen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"pen\": " + SavePenToStr(pen);
        }

    // brush (skip if default white solid for Shape)
    const auto& brush = shape->GetBrush();
    if (brush.IsOk() && brush != wxNullBrush &&
        !(brush.GetColour() == *wxWHITE && brush.GetStyle() == wxBRUSHSTYLE_SOLID))
        {
        tmpl += L", \"brush\": " + SaveBrushToStr(brush);
        }

    // label (as sub-object if it has color/templates, otherwise plain string)
    if (!shape->GetText().empty())
        {
        const auto labelTextTmpl = shape->GetPropertyTemplate(L"label.text");
        const auto labelColorTmpl = shape->GetPropertyTemplate(L"label.color");
        const auto& labelText = labelTextTmpl.empty() ? shape->GetText() : labelTextTmpl;

        if (!labelColorTmpl.empty() ||
            (shape->GetFontColor().IsOk() && shape->GetFontColor() != *wxBLACK))
            {
            const wxString colorVal =
                labelColorTmpl.empty() ? ColorToStr(shape->GetFontColor()) : labelColorTmpl;
            tmpl += L", \"label\": {\"text\": \"" + EscapeJsonStr(labelText) +
                    L"\", \"color\": \"" + EscapeJsonStr(colorVal) + L"\"}";
            }
        else
            {
            tmpl += L", \"label\": \"" + EscapeJsonStr(labelText) + L"\"";
            }
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    SaveItem(node, shape, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SaveFillableShape(const Wisteria::GraphItems::FillableShape* shape,
                                                   const Wisteria::Canvas* canvas) const
    {
    if (shape == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    const auto iconStr = Wisteria::ReportEnumConvert::ConvertIconToString(shape->GetShape());

    wxString tmpl = L"{\"type\": \"fillable-shape\"";
    if (iconStr.has_value())
        {
        tmpl += L", \"icon\": \"" + iconStr.value() + L"\"";
        }

    // size (prefer cached original values, fall back to current)
    const auto widthStr = shape->GetPropertyTemplate(L"size.width");
    const auto heightStr = shape->GetPropertyTemplate(L"size.height");
    if (!widthStr.empty() || !heightStr.empty())
        {
        tmpl += L", \"size\": {";
        bool needComma = false;
        if (!widthStr.empty())
            {
            tmpl += L"\"width\": " + widthStr;
            needComma = true;
            }
        if (!heightStr.empty())
            {
            if (needComma)
                {
                tmpl += L", ";
                }
            tmpl += L"\"height\": " + heightStr;
            }
        tmpl += L"}";
        }
    else
        {
        const auto sz = shape->GetSizeDIPS();
        tmpl += wxString::Format(L", \"size\": {\"width\": %d, \"height\": %d}", sz.GetWidth(),
                                 sz.GetHeight());
        }

    // pen (skip if default black solid width-1)
    const auto& pen = shape->GetPen();
    if (pen.IsOk() && pen != wxNullPen &&
        !(pen.GetColour() == *wxBLACK && pen.GetWidth() <= 1 && pen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"pen\": " + SavePenToStr(pen);
        }

    // brush (skip if default black solid for FillableShape)
    const auto& brush = shape->GetBrush();
    if (brush.IsOk() && brush != wxNullBrush &&
        !(brush.GetColour() == *wxBLACK && brush.GetStyle() == wxBRUSHSTYLE_SOLID))
        {
        tmpl += L", \"brush\": " + SaveBrushToStr(brush);
        }

    // label (as sub-object if it has color/templates, otherwise plain string)
    if (!shape->GetText().empty())
        {
        const auto labelTextTmpl = shape->GetPropertyTemplate(L"label.text");
        const auto labelColorTmpl = shape->GetPropertyTemplate(L"label.color");
        const auto& labelText = labelTextTmpl.empty() ? shape->GetText() : labelTextTmpl;

        if (!labelColorTmpl.empty() ||
            (shape->GetFontColor().IsOk() && shape->GetFontColor() != *wxBLACK))
            {
            const wxString colorVal =
                labelColorTmpl.empty() ? ColorToStr(shape->GetFontColor()) : labelColorTmpl;
            tmpl += L", \"label\": {\"text\": \"" + EscapeJsonStr(labelText) +
                    L"\", \"color\": \"" + EscapeJsonStr(colorVal) + L"\"}";
            }
        else
            {
            tmpl += L", \"label\": \"" + EscapeJsonStr(labelText) + L"\"";
            }
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    // fill-percent (prefer template for {{constants}})
    const auto fpTemplate = shape->GetPropertyTemplate(L"fill-percent");
    if (!fpTemplate.empty())
        {
        node->Add(L"fill-percent", fpTemplate);
        }
    else if (!compare_doubles(shape->GetFillPercent(), math_constants::empty))
        {
        node->Add(L"fill-percent", shape->GetFillPercent());
        }

    SaveItem(node, shape, canvas);
    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SaveCommonAxis(const Wisteria::GraphItems::Axis* axis,
                                                const Wisteria::Canvas* canvas) const
    {
    if (axis == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    wxString tmpl = L"{\"type\": \"common-axis\"";

    // axis-type
    const auto axisTypeStr =
        Wisteria::ReportEnumConvert::ConvertAxisTypeToString(axis->GetAxisType());
    if (axisTypeStr.has_value())
        {
        tmpl += L", \"axis-type\": \"" + axisTypeStr.value() + L"\"";
        }

    // child-ids (cached as comma-separated string)
    const auto childIdsStr = axis->GetPropertyTemplate(L"child-ids");
    if (!childIdsStr.empty())
        {
        tmpl += L", \"child-ids\": [" + childIdsStr + L"]";
        }

    // common-perpendicular-axis
    const auto cpaStr = axis->GetPropertyTemplate(L"common-perpendicular-axis");
    if (cpaStr == L"true")
        {
        tmpl += L", \"common-perpendicular-axis\": true";
        }

    // title
    const auto& title = axis->GetTitle();
    if (!title.GetText().empty() || title.IsShown())
        {
        const auto textTemplate = title.GetPropertyTemplate(L"text");
        const auto& titleText = textTemplate.empty() ? title.GetText() : textTemplate;
        tmpl += L", \"title\": {\"text\": \"" + EscapeJsonStr(titleText) + L"\"}";
        }

    // axis-pen
    const auto& axisPen = axis->GetAxisLinePen();
    if (!axisPen.IsOk() || axisPen == wxNullPen)
        {
        tmpl += L", \"axis-pen\": null";
        }
    else if (!(axisPen.GetColour() == *wxBLACK && axisPen.GetWidth() <= 1 &&
               axisPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"axis-pen\": " + SavePenToStr(axisPen);
        }

    // gridline-pen
    const auto& gridPen = axis->GetGridlinePen();
    if (!gridPen.IsOk() || gridPen == wxNullPen)
        {
        tmpl += L", \"gridline-pen\": null";
        }
    else if (!(gridPen.GetColour() == *wxBLACK && gridPen.GetWidth() <= 1 &&
               gridPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        tmpl += L", \"gridline-pen\": " + SavePenToStr(gridPen);
        }

    // label-display
    const auto ldStr =
        Wisteria::ReportEnumConvert::ConvertAxisLabelDisplayToString(axis->GetLabelDisplay());
    if (ldStr.has_value() &&
        axis->GetLabelDisplay() != Wisteria::AxisLabelDisplay::DisplayCustomLabelsOrValues)
        {
        tmpl += L", \"label-display\": \"" + ldStr.value() + L"\"";
        }

    // number-display
    const auto numDisplayStr =
        Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(axis->GetNumberDisplay());
    if (numDisplayStr.has_value() && axis->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
        {
        tmpl += L", \"number-display\": \"" + numDisplayStr.value() + L"\"";
        }

    // tickmarks
    const auto tmStr =
        Wisteria::ReportEnumConvert::ConvertTickMarkDisplayToString(axis->GetTickMarkDisplay());
    if (tmStr.has_value() &&
        axis->GetTickMarkDisplay() != Wisteria::GraphItems::Axis::TickMark::DisplayType::Inner)
        {
        tmpl += L", \"tickmarks\": {\"display\": \"" + tmStr.value() + L"\"}";
        }

    // double-sided-labels (default is false)
    if (axis->HasDoubleSidedAxisLabels())
        {
        tmpl += L", \"double-sided-labels\": true";
        }

    // range
    const auto [rangeStart, rangeEnd] = axis->GetRange();
    if (!compare_doubles(rangeStart, 0.0) || !compare_doubles(rangeEnd, 0.0))
        {
        tmpl += wxString::Format(L", \"range\": {\"start\": %g, \"end\": %g", rangeStart, rangeEnd);
        if (axis->GetPrecision() != 0)
            {
            tmpl +=
                wxString::Format(L", \"precision\": %d", static_cast<int>(axis->GetPrecision()));
            }
        if (!compare_doubles(axis->GetInterval(), 0.0))
            {
            tmpl += wxString::Format(L", \"interval\": %g", axis->GetInterval());
            }
        if (axis->GetDisplayInterval() != 1)
            {
            tmpl += wxString::Format(L", \"display-interval\": %zu", axis->GetDisplayInterval());
            }
        tmpl += L"}";
        }

    // precision (outside of range)
    if (axis->GetPrecision() != 0 &&
        (compare_doubles(rangeStart, 0.0) && compare_doubles(rangeEnd, 0.0)))
        {
        tmpl += wxString::Format(L", \"precision\": %d", static_cast<int>(axis->GetPrecision()));
        }

    // label-length
    if (axis->GetLabelLineLength() != 100)
        {
        tmpl += wxString::Format(L", \"label-length\": %zu", axis->GetLabelLineLength());
        }

    // custom-labels: only serialize when they are a user override (set in
    // JSON or the label editor). Dataset-derived labels, which flow in via
    // CommonAxisBuilder copying from a child, are rebuilt on load and would
    // go stale if persisted.
    const auto& customLabels = axis->GetCustomLabels();
    if (axis->AreCustomLabelsUserOverride() && !customLabels.empty())
        {
        tmpl += L", \"custom-labels\": [";
        bool first = true;
        for (const auto& [value, label] : customLabels)
            {
            if (!first)
                {
                tmpl += L", ";
                }
            first = false;
            const auto labelTextTmpl = label.GetPropertyTemplate(L"text");
            const auto& labelText = labelTextTmpl.empty() ? label.GetText() : labelTextTmpl;
            tmpl += wxString::Format(L"{\"value\": %g, \"label\": \"%s\"}", value,
                                     EscapeJsonStr(labelText));
            }
        tmpl += L"]";
        }

    // brackets
    const auto& brackets = axis->GetBrackets();
    if (!brackets.empty())
        {
        const auto bracketDsName = axis->GetPropertyTemplate(L"brackets.dataset");
        if (!bracketDsName.empty())
            {
            // dataset-based brackets
            tmpl += L", \"brackets\": {\"dataset\": \"" + EscapeJsonStr(bracketDsName) + L"\"";
            if (axis->AreBracketsSimplified())
                {
                tmpl += L", \"simplify\": true";
                }
            const auto labelVar = axis->GetPropertyTemplate(L"bracket.label");
            const auto valueVar = axis->GetPropertyTemplate(L"bracket.value");
            if (!labelVar.empty() || !valueVar.empty())
                {
                tmpl += L", \"variables\": {";
                bool needComma = false;
                if (!labelVar.empty())
                    {
                    tmpl += L"\"label\": \"" + EscapeJsonStr(labelVar) + L"\"";
                    needComma = true;
                    }
                if (!valueVar.empty())
                    {
                    if (needComma)
                        {
                        tmpl += L", ";
                        }
                    tmpl += L"\"value\": \"" + EscapeJsonStr(valueVar) + L"\"";
                    }
                tmpl += L"}";
                }
            // pen from first bracket
            if (!brackets.empty())
                {
                const auto& bPen = brackets[0].GetLinePen();
                if (bPen.IsOk() && bPen != wxNullPen &&
                    !(bPen.GetColour() == *wxBLACK && bPen.GetWidth() <= 2 &&
                      bPen.GetStyle() == wxPENSTYLE_SOLID))
                    {
                    tmpl += L", \"pen\": " + SavePenToStr(bPen);
                    }
                }
            // style from first bracket
            if (!brackets.empty())
                {
                const auto bsStr = Wisteria::ReportEnumConvert::ConvertBracketLineStyleToString(
                    brackets[0].GetBracketLineStyle());
                if (bsStr.has_value() &&
                    brackets[0].GetBracketLineStyle() != Wisteria::BracketLineStyle::CurlyBraces)
                    {
                    tmpl += L", \"style\": \"" + bsStr.value() + L"\"";
                    }
                }
            tmpl += L"}";
            }
        else
            {
            // individually defined brackets
            tmpl += L", \"brackets\": [";
            for (size_t i = 0; i < brackets.size(); ++i)
                {
                if (i > 0)
                    {
                    tmpl += L", ";
                    }
                const auto& b = brackets[i];
                tmpl += wxString::Format(L"{\"start\": %g, \"end\": %g", b.GetStartPosition(),
                                         b.GetEndPosition());
                if (!b.GetLabel().GetText().empty())
                    {
                    tmpl += L", \"label\": \"" + EscapeJsonStr(b.GetLabel().GetText()) + L"\"";
                    }
                const auto& bPen = b.GetLinePen();
                if (bPen.IsOk() && bPen != wxNullPen &&
                    !(bPen.GetColour() == *wxBLACK && bPen.GetWidth() <= 2 &&
                      bPen.GetStyle() == wxPENSTYLE_SOLID))
                    {
                    tmpl += L", \"pen\": " + SavePenToStr(bPen);
                    }
                const auto bsStr = Wisteria::ReportEnumConvert::ConvertBracketLineStyleToString(
                    b.GetBracketLineStyle());
                if (bsStr.has_value() &&
                    b.GetBracketLineStyle() != Wisteria::BracketLineStyle::CurlyBraces)
                    {
                    tmpl += L", \"style\": \"" + bsStr.value() + L"\"";
                    }
                tmpl += L"}";
                }
            tmpl += L"]";
            }
        }

    // show (default is true)
    if (!axis->IsShown())
        {
        tmpl += L", \"show\": false";
        }

    // show-outer-labels (default is true)
    if (!axis->IsShowingOuterLabels())
        {
        tmpl += L", \"show-outer-labels\": false";
        }

    tmpl += L", \"canvas-margins\": [], \"padding\": [], \"outline\": []";
    tmpl += L"}";

    auto node = wxSimpleJSON::Create(tmpl);

    SaveItem(node, axis, canvas);
    return node;
    }

//-------------------------------------------
void WisteriaDoc::SaveDatasetImportOptions(
    const wxSimpleJSON::Ptr_t& dsNode, const Wisteria::Data::Dataset::ColumnPreviewInfo& colInfo,
    const Wisteria::Data::ImportInfo& info,
    const std::vector<Wisteria::ReportBuilder::DatasetColumnRename>& columnRenames)
    {
    using CIT = Wisteria::Data::Dataset::ColumnImportType;

    // returns the original (pre-rename) name for a column that may have been renamed
    const auto originalColName = [&columnRenames](const wxString& currentName) -> wxString
    {
        for (const auto& rename : columnRenames)
            {
            if (!rename.m_name.empty() && currentName.CmpNoCase(rename.m_newName) == 0)
                {
                return rename.m_name;
                }
            }
        return currentName;
    };

    // import settings
    if (info.GetSkipRows() > 0)
        {
        dsNode->Add(L"skip-rows", static_cast<double>(info.GetSkipRows()));
        }
    if (!std::isnan(info.GetContinuousMDRecodeValue()))
        {
        dsNode->Add(L"continuous-md-recode-value", info.GetContinuousMDRecodeValue());
        }
    if (info.GetMDCodes().has_value())
        {
        wxArrayString mdArr;
        for (const auto& code : info.GetMDCodes().value())
            {
            mdArr.Add(wxString{ code });
            }
        dsNode->Add(L"md-codes", mdArr);
        }
    if (info.GetTreatLeadingZerosAsText())
        {
        dsNode->Add(L"treat-leading-zeros-as-text", true);
        }
    if (info.GetTreatYearsAsText())
        {
        dsNode->Add(L"treat-years-as-text", true);
        }
    if (info.GetMaxDiscreteValue() != 7)
        {
        dsNode->Add(L"max-discrete-value", static_cast<double>(info.GetMaxDiscreteValue()));
        }

    // id column
    if (!info.GetIdColumn().empty())
        {
        dsNode->Add(L"id-column", info.GetIdColumn());
        }

    // continuous columns
    wxArrayString contCols;
    for (const auto& col : colInfo)
        {
        if (!col.m_excluded && col.m_type == CIT::Numeric)
            {
            contCols.Add(originalColName(col.m_name));
            }
        }
    if (!contCols.empty())
        {
        dsNode->Add(L"continuous-columns", contCols);
        }

    // categorical columns
    auto catArray = dsNode->GetProperty(L"categorical-columns");
    for (const auto& col : colInfo)
        {
        if (col.m_excluded)
            {
            continue;
            }
        if (col.m_type == CIT::String || col.m_type == CIT::Discrete ||
            col.m_type == CIT::DichotomousString || col.m_type == CIT::DichotomousDiscrete)
            {
            auto catObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            const auto origCatName = originalColName(col.m_name);
            catObj->Add(L"name", origCatName);
            const auto catIt =
                std::ranges::find_if(info.GetCategoricalColumns(), [&origCatName](const auto& ci)
                                     { return ci.m_columnName == origCatName; });
            if (catIt != info.GetCategoricalColumns().cend() &&
                catIt->m_importMethod == Wisteria::Data::CategoricalImportMethod::ReadAsIntegers)
                {
                catObj->Add(L"parser", wxString{ L"as-integers" });
                }
            catArray->ArrayAdd(catObj);
            }
        }

    // date columns
    auto dateArray = dsNode->GetProperty(L"date-columns");
    for (const auto& col : colInfo)
        {
        if (!col.m_excluded && col.m_type == CIT::Date)
            {
            auto dateObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            const auto origDateName = originalColName(col.m_name);
            dateObj->Add(L"name", origDateName);
            for (const auto& di : info.GetDateColumns())
                {
                if (di.m_columnName == origDateName)
                    {
                    switch (di.m_importMethod)
                        {
                    case Wisteria::Data::DateImportMethod::IsoDate:
                        dateObj->Add(L"parser", wxString{ L"iso-date" });
                        break;
                    case Wisteria::Data::DateImportMethod::IsoCombined:
                        dateObj->Add(L"parser", wxString{ L"iso-combined" });
                        break;
                    case Wisteria::Data::DateImportMethod::Rfc822:
                        dateObj->Add(L"parser", wxString{ L"rfc822" });
                        break;
                    case Wisteria::Data::DateImportMethod::StrptimeFormatString:
                        dateObj->Add(L"parser", wxString{ L"strptime-format" });
                        if (!di.m_strptimeFormatString.empty())
                            {
                            dateObj->Add(L"format", di.m_strptimeFormatString);
                            }
                        break;
                    case Wisteria::Data::DateImportMethod::Time:
                        dateObj->Add(L"parser", wxString{ L"time" });
                        break;
                    case Wisteria::Data::DateImportMethod::Automatic:
                        [[fallthrough]];
                    default:
                        break;
                        }
                    break;
                    }
                }
            dateArray->ArrayAdd(dateObj);
            }
        }

    // columns-order (preserves original spreadsheet column ordering)
    wxArrayString colOrder;
    for (const auto& col : colInfo)
        {
        if (!col.m_excluded)
            {
            colOrder.Add(originalColName(col.m_name));
            }
        }
    if (!colOrder.empty())
        {
        dsNode->Add(L"columns-order", colOrder);
        }
    }

//-------------------------------------------
void WisteriaDoc::SaveTransformOptions(
    const wxSimpleJSON::Ptr_t& dsNode,
    const Wisteria::ReportBuilder::DatasetTransformOptions& txOpts)
    {
    if (!txOpts.m_recodeREs.empty())
        {
        auto recodeArray = dsNode->GetProperty(L"recode-re");
        for (const auto& rr : txOpts.m_recodeREs)
            {
            auto rrObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            rrObj->Add(L"column", rr.m_column);
            rrObj->Add(L"pattern", rr.m_pattern);
            rrObj->Add(L"replacement", rr.m_replacement);
            recodeArray->ArrayAdd(rrObj);
            }
        }

    if (!txOpts.m_columnRenames.empty())
        {
        auto renameArray = dsNode->GetProperty(L"columns-rename");
        for (const auto& cr : txOpts.m_columnRenames)
            {
            auto crObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            if (!cr.m_nameRe.empty())
                {
                crObj->Add(L"name-re", cr.m_nameRe);
                crObj->Add(L"new-name-re", cr.m_newNameRe);
                }
            else
                {
                crObj->Add(L"name", cr.m_name);
                crObj->Add(L"new-name", cr.m_newName);
                }
            renameArray->ArrayAdd(crObj);
            }
        }

    if (!txOpts.m_mutateCategoricalColumns.empty())
        {
        auto mutArray = dsNode->GetProperty(L"mutate-categorical-columns");
        for (const auto& mc : txOpts.m_mutateCategoricalColumns)
            {
            auto mcObj = wxSimpleJSON::Create(L"{\"replacements\": []}");
            mcObj->Add(L"source-column", mc.m_sourceColumn);
            mcObj->Add(L"target-column", mc.m_targetColumn);
            auto replArray = mcObj->GetProperty(L"replacements");
            for (const auto& [pattern, replacement] : mc.m_replacements)
                {
                auto rObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
                rObj->Add(L"pattern", pattern);
                rObj->Add(L"replacement", replacement);
                replArray->ArrayAdd(rObj);
                }
            mutArray->ArrayAdd(mcObj);
            }
        }

    if (!txOpts.m_columnsSelect.empty())
        {
        dsNode->Add(L"columns-select", txOpts.m_columnsSelect);
        }

    if (!txOpts.m_collapseMins.empty())
        {
        auto cmArray = dsNode->GetProperty(L"collapse-min");
        for (const auto& cm : txOpts.m_collapseMins)
            {
            auto cmObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            cmObj->Add(L"column", cm.m_column);
            cmObj->Add(L"min", cm.m_min);
            if (!cm.m_otherLabel.empty())
                {
                cmObj->Add(L"other-label", cm.m_otherLabel);
                }
            cmArray->ArrayAdd(cmObj);
            }
        }

    if (!txOpts.m_collapseExcepts.empty())
        {
        auto ceArray = dsNode->GetProperty(L"collapse-except");
        for (const auto& ce : txOpts.m_collapseExcepts)
            {
            auto ceObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
            ceObj->Add(L"column", ce.m_column);
            wxArrayString labels;
            for (const auto& lbl : ce.m_labelsToKeep)
                {
                labels.Add(lbl);
                }
            ceObj->Add(L"labels-to-keep", labels);
            if (!ce.m_otherLabel.empty())
                {
                ceObj->Add(L"other-label", ce.m_otherLabel);
                }
            ceArray->ArrayAdd(ceObj);
            }
        }

    if (txOpts.m_columnNamesSort)
        {
        dsNode->Add(L"column-names-sort", true);
        }
    }

//-------------------------------------------
void WisteriaDoc::SaveFormulas(
    const wxSimpleJSON::Ptr_t& dsNode,
    const std::vector<Wisteria::ReportBuilder::DatasetFormulaInfo>& formulas)
    {
    if (formulas.empty())
        {
        return;
        }
    auto formulaArray = dsNode->GetProperty(L"formulas");
    for (const auto& f : formulas)
        {
        auto fObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
        fObj->Add(L"name", f.m_name);
        fObj->Add(L"value", f.m_value);
        formulaArray->ArrayAdd(fObj);
        }
    }

//-------------------------------------------
void WisteriaDoc::SaveSubsetFilters(const wxSimpleJSON::Ptr_t& subsetNode,
                                    const Wisteria::ReportBuilder::DatasetSubsetOptions& sOpts)
    {
    using FT = Wisteria::ReportBuilder::DatasetSubsetOptions::FilterType;

    const auto saveFilterObj = [](const Wisteria::ReportBuilder::DatasetFilterInfo& fi)
    {
        auto fObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
        fObj->Add(L"column", fi.m_column);
        if (fi.m_operator != L"=")
            {
            fObj->Add(L"operator", fi.m_operator);
            }
        wxArrayString vals;
        for (const auto& v : fi.m_values)
            {
            vals.Add(v);
            }
        fObj->Add(L"values", vals);
        return fObj;
    };

    if (sOpts.m_filterType == FT::Section)
        {
        auto secObj = subsetNode->GetProperty(L"section");
        secObj->Add(L"column", sOpts.m_sectionColumn);
        secObj->Add(L"start-label", sOpts.m_sectionStartLabel);
        secObj->Add(L"end-label", sOpts.m_sectionEndLabel);
        if (!sOpts.m_sectionIncludeSentinelLabels)
            {
            secObj->Add(L"include-sentinel-labels", false);
            }
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-and");
        subsetNode->DeleteProperty(L"filter-or");
        }
    else if (sOpts.m_filterType == FT::Single && sOpts.m_filters.size() == 1)
        {
        const auto& fi = sOpts.m_filters[0];
        auto filterObj = subsetNode->GetProperty(L"filter");
        filterObj->Add(L"column", fi.m_column);
        if (fi.m_operator != L"=")
            {
            filterObj->Add(L"operator", fi.m_operator);
            }
        wxArrayString vals;
        for (const auto& v : fi.m_values)
            {
            vals.Add(v);
            }
        filterObj->Add(L"values", vals);
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter-and");
        subsetNode->DeleteProperty(L"filter-or");
        }
    else if (sOpts.m_filterType == FT::And)
        {
        auto andArray = subsetNode->GetProperty(L"filter-and");
        for (const auto& fi : sOpts.m_filters)
            {
            andArray->ArrayAdd(saveFilterObj(fi));
            }
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-or");
        }
    else if (sOpts.m_filterType == FT::Or)
        {
        auto orArray = subsetNode->GetProperty(L"filter-or");
        for (const auto& fi : sOpts.m_filters)
            {
            orArray->ArrayAdd(saveFilterObj(fi));
            }
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-and");
        }
    else
        {
        subsetNode->DeleteProperty(L"section");
        subsetNode->DeleteProperty(L"filter");
        subsetNode->DeleteProperty(L"filter-and");
        subsetNode->DeleteProperty(L"filter-or");
        }
    }

//-------------------------------------------
void WisteriaDoc::SaveSubsets(const wxSimpleJSON::Ptr_t& parentNode,
                              const wxString& sourceName) const
    {
    auto* view = dynamic_cast<WisteriaView*>(GetFirstView());
    if (view == nullptr)
        {
        wxASSERT_MSG(view, L"Invalid view connected to document?!");
        return;
        }
    const auto& subsetOpts = view->GetReportBuilder().GetDatasetSubsetOptions();
    const auto& transformOpts = view->GetReportBuilder().GetDatasetTransformOptions();

    auto subArray = parentNode->GetProperty(L"subsets");
    for (const auto& [subName, sOpts] : subsetOpts)
        {
        if (sOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0 &&
            subName.CmpNoCase(sourceName) != 0)
            {
            auto subObj = wxSimpleJSON::Create(wxString::Format(
                L"{\"name\": \"%s\", "
                L"\"section\": {}, \"filter\": {}, \"filter-and\": [], \"filter-or\": [], "
                L"\"subsets\": [], \"pivots\": [], \"merges\": [], "
                L"\"recode-re\": [], \"columns-rename\": [], "
                L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                L"\"collapse-except\": [], \"formulas\": []}",
                EscapeJsonStr(subName)));
            SaveSubsetFilters(subObj, sOpts);

            SaveSubsets(subObj, subName);
            SavePivots(subObj, subName);
            SaveMerges(subObj, subName);

            const auto txIt = transformOpts.find(subName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(subObj, txIt->second);
                SaveFormulas(subObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"subsets", L"pivots", L"merges", L"recode-re",
                                     L"columns-rename", L"mutate-categorical-columns",
                                     L"collapse-min", L"collapse-except", L"formulas" })
                {
                if (subObj->GetProperty(key)->ArraySize() == 0)
                    {
                    subObj->DeleteProperty(key);
                    }
                }

            subArray->ArrayAdd(subObj);
            }
        }
    }

//-------------------------------------------
void WisteriaDoc::SavePivots(const wxSimpleJSON::Ptr_t& parentNode,
                             const wxString& sourceName) const
    {
    auto* view = dynamic_cast<WisteriaView*>(GetFirstView());
    if (view == nullptr)
        {
        wxASSERT_MSG(view, L"Invalid view connected to document?!");
        return;
        }
    const auto& pivotOpts = view->GetReportBuilder().GetDatasetPivotOptions();
    const auto& transformOpts = view->GetReportBuilder().GetDatasetTransformOptions();

    auto pivArray = parentNode->GetProperty(L"pivots");
    for (const auto& [pivName, pOpts] : pivotOpts)
        {
        if (pOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0 &&
            pivName.CmpNoCase(sourceName) != 0)
            {
            auto pivObj = wxSimpleJSON::Create(
                pivName.empty() ?
                    wxString(L"{\"subsets\": [], \"pivots\": [], \"merges\": [], "
                             L"\"recode-re\": [], \"columns-rename\": [], "
                             L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                             L"\"collapse-except\": [], \"formulas\": []}") :
                    wxString::Format(L"{\"name\": \"%s\", "
                                     L"\"subsets\": [], \"pivots\": [], \"merges\": [], "
                                     L"\"recode-re\": [], \"columns-rename\": [], "
                                     L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                                     L"\"collapse-except\": [], \"formulas\": []}",
                                     EscapeJsonStr(pivName)));

            if (pOpts.m_type == Wisteria::ReportBuilder::PivotType::Wider)
                {
                pivObj->Add(L"type", wxString{ L"wider" });
                if (!pOpts.m_idColumns.empty())
                    {
                    wxArrayString ids;
                    for (const auto& id : pOpts.m_idColumns)
                        {
                        ids.Add(id);
                        }
                    pivObj->Add(L"id-columns", ids);
                    }
                if (!pOpts.m_namesFromColumn.empty())
                    {
                    pivObj->Add(L"names-from-column", pOpts.m_namesFromColumn);
                    }
                if (!pOpts.m_valuesFromColumns.empty())
                    {
                    wxArrayString vals;
                    for (const auto& v : pOpts.m_valuesFromColumns)
                        {
                        vals.Add(v);
                        }
                    pivObj->Add(L"values-from-columns", vals);
                    }
                if (!pOpts.m_namesSep.empty() && pOpts.m_namesSep != L"_")
                    {
                    pivObj->Add(L"names-separator", pOpts.m_namesSep);
                    }
                if (!pOpts.m_namesPrefix.empty())
                    {
                    pivObj->Add(L"names-prefix", pOpts.m_namesPrefix);
                    }
                if (!std::isnan(pOpts.m_fillValue))
                    {
                    pivObj->Add(L"fill-value", pOpts.m_fillValue);
                    }
                }
            else
                {
                pivObj->Add(L"type", wxString{ L"longer" });
                if (!pOpts.m_columnsToKeep.empty())
                    {
                    wxArrayString cols;
                    for (const auto& c : pOpts.m_columnsToKeep)
                        {
                        cols.Add(c);
                        }
                    pivObj->Add(L"columns-to-keep", cols);
                    }
                if (!pOpts.m_fromColumns.empty())
                    {
                    wxArrayString cols;
                    for (const auto& c : pOpts.m_fromColumns)
                        {
                        cols.Add(c);
                        }
                    pivObj->Add(L"from-columns", cols);
                    }
                if (!pOpts.m_namesTo.empty())
                    {
                    wxArrayString cols;
                    for (const auto& c : pOpts.m_namesTo)
                        {
                        cols.Add(c);
                        }
                    pivObj->Add(L"names-to", cols);
                    }
                if (!pOpts.m_valuesTo.empty())
                    {
                    pivObj->Add(L"values-to", pOpts.m_valuesTo);
                    }
                if (!pOpts.m_namesPattern.empty())
                    {
                    pivObj->Add(L"names-pattern", pOpts.m_namesPattern);
                    }
                }

            SaveSubsets(pivObj, pivName);
            SavePivots(pivObj, pivName);
            SaveMerges(pivObj, pivName);

            const auto txIt = transformOpts.find(pivName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(pivObj, txIt->second);
                SaveFormulas(pivObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"subsets", L"merges", L"pivots", L"recode-re",
                                     L"columns-rename", L"mutate-categorical-columns",
                                     L"collapse-min", L"collapse-except", L"formulas" })
                {
                if (pivObj->GetProperty(key)->ArraySize() == 0)
                    {
                    pivObj->DeleteProperty(key);
                    }
                }

            pivArray->ArrayAdd(pivObj);
            }
        }
    }

//-------------------------------------------
void WisteriaDoc::SaveMerges(const wxSimpleJSON::Ptr_t& parentNode,
                             const wxString& sourceName) const
    {
    auto* view = dynamic_cast<WisteriaView*>(GetFirstView());
    if (view == nullptr)
        {
        wxASSERT_MSG(view, L"Invalid view connected to document?!");
        return;
        }
    const auto& mergeOpts = view->GetReportBuilder().GetDatasetMergeOptions();
    const auto& transformOpts = view->GetReportBuilder().GetDatasetTransformOptions();

    auto mrgArray = parentNode->GetProperty(L"merges");
    for (const auto& [mrgName, mOpts] : mergeOpts)
        {
        if (mOpts.m_sourceDatasetName.CmpNoCase(sourceName) == 0 &&
            mrgName.CmpNoCase(sourceName) != 0)
            {
            auto mrgObj = wxSimpleJSON::Create(
                wxString::Format(L"{\"name\": \"%s\", \"by\": [], "
                                 L"\"subsets\": [], \"pivots\": [], \"merges\": [], "
                                 L"\"recode-re\": [], \"columns-rename\": [], "
                                 L"\"mutate-categorical-columns\": [], \"collapse-min\": [], "
                                 L"\"collapse-except\": [], \"formulas\": []}",
                                 EscapeJsonStr(mrgName)));
            if (!mOpts.m_type.empty() && mOpts.m_type != L"left-join-unique-last")
                {
                mrgObj->Add(L"type", mOpts.m_type);
                }
            mrgObj->Add(L"other-dataset", mOpts.m_otherDatasetName);
            if (!mOpts.m_byColumns.empty())
                {
                auto byArray = mrgObj->GetProperty(L"by");
                for (const auto& [left, right] : mOpts.m_byColumns)
                    {
                    auto byObj = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
                    byObj->Add(L"left-column", left);
                    byObj->Add(L"right-column", right);
                    byArray->ArrayAdd(byObj);
                    }
                }
            else
                {
                mrgObj->DeleteProperty(L"by");
                }
            if (!mOpts.m_suffix.empty() && mOpts.m_suffix != L".x")
                {
                mrgObj->Add(L"suffix", mOpts.m_suffix);
                }

            SaveSubsets(mrgObj, mrgName);
            SavePivots(mrgObj, mrgName);
            SaveMerges(mrgObj, mrgName);

            const auto txIt = transformOpts.find(mrgName);
            if (txIt != transformOpts.cend())
                {
                SaveTransformOptions(mrgObj, txIt->second);
                SaveFormulas(mrgObj, txIt->second.m_formulas);
                }

            // clean up empty arrays
            for (const auto& key : { L"subsets", L"pivots", L"merges", L"recode-re",
                                     L"columns-rename", L"mutate-categorical-columns",
                                     L"collapse-min", L"collapse-except", L"formulas" })
                {
                if (mrgObj->GetProperty(key)->ArraySize() == 0)
                    {
                    mrgObj->DeleteProperty(key);
                    }
                }

            mrgArray->ArrayAdd(mrgObj);
            }
        }
    }

//-------------------------------------------
void WisteriaDoc::SaveGraph(const Wisteria::Graphs::Graph2D* graph, wxSimpleJSON::Ptr_t& graphNode,
                            const Wisteria::Canvas* canvas) const
    {
    if (graph == nullptr || canvas == nullptr)
        {
        return;
        }

    // dataset name (from cached property template)
    const wxString datasetName = graph->GetPropertyTemplate(L"dataset");

    // variables (from cached property templates)
    // indexed variables like y[0], y[1] are collapsed into "y": [...]
    const auto& templates = graph->GetPropertyTemplates();
    wxString varsStr;
    std::map<wxString, std::vector<std::pair<size_t, wxString>>> indexedVars;
    for (const auto& [prop, val] : templates)
        {
        if (prop.StartsWith(L"variables."))
            {
            const auto varName = prop.Mid(10); // skip "variables."
            // check for indexed pattern like "y[0]"
            const auto bracketPos = varName.find(L'[');
            if (bracketPos != wxString::npos && varName.EndsWith(L"]"))
                {
                const auto baseName = varName.Left(bracketPos);
                const auto idxStr = varName.Mid(bracketPos + 1, varName.length() - bracketPos - 2);
                unsigned long idx = 0;
                if (idxStr.ToULong(&idx))
                    {
                    indexedVars[baseName].emplace_back(static_cast<size_t>(idx), val);
                    }
                }
            else
                {
                if (!varsStr.empty())
                    {
                    varsStr += L", ";
                    }
                varsStr += L"\"" + varName + L"\": \"" + EscapeJsonStr(val) + L"\"";
                }
            }
        }
    // write indexed variables as arrays
    for (auto& [baseName, entries] : indexedVars)
        {
        std::ranges::sort(entries, [](const auto& a, const auto& b) { return a.first < b.first; });
        if (!varsStr.empty())
            {
            varsStr += L", ";
            }
        varsStr += L"\"" + baseName + L"\": [";
        for (size_t i = 0; i < entries.size(); ++i)
            {
            if (i > 0)
                {
                varsStr += L", ";
                }
            varsStr += L"\"" + EscapeJsonStr(entries[i].second) + L"\"";
            }
        varsStr += L"]";
        }

    // title
    wxString titleStr;
    if (!graph->GetTitle().GetText().empty())
        {
        titleStr = SaveLabelPropertiesToStr(graph->GetTitle());
        }

    // subtitle
    wxString subtitleStr;
    if (!graph->GetSubtitle().GetText().empty())
        {
        subtitleStr = SaveLabelPropertiesToStr(graph->GetSubtitle());
        }

    // caption
    wxString captionStr;
    if (!graph->GetCaption().GetText().empty())
        {
        captionStr = SaveLabelPropertiesToStr(graph->GetCaption());
        }

    // background-color
    const auto& bgColor = graph->GetPlotBackgroundColor();
    if (bgColor.IsOk() && !bgColor.IsTransparent())
        {
        graphNode->Add(L"background-color", ColorToStr(bgColor));
        }

    // background-image
    const auto& bgImgBundle = graph->GetPlotBackgroundImage();
    if (bgImgBundle.IsOk())
        {
        wxString bgImgStr = L"{";
        const auto pathsTemplate = graph->GetPropertyTemplate(L"image-import.paths");
        const auto pathTemplate = graph->GetPropertyTemplate(L"image-import.path");
        const auto effectTemplate = graph->GetPropertyTemplate(L"image-import.effect");
        const auto stitchTemplate = graph->GetPropertyTemplate(L"image-import.stitch");

        if (!pathsTemplate.empty())
            {
            bgImgStr += L"\"image-import\": {\"paths\": [";
            wxStringTokenizer tokenizer(pathsTemplate, L"\t");
            bool first = true;
            while (tokenizer.HasMoreTokens())
                {
                if (!first)
                    {
                    bgImgStr += L", ";
                    }
                bgImgStr +=
                    L"\"" + EscapeJsonStr(MakeRelativePath(tokenizer.GetNextToken())) + L"\"";
                first = false;
                }
            bgImgStr += L"]";
            if (!stitchTemplate.empty())
                {
                bgImgStr += L", \"stitch\": \"" + EscapeJsonStr(stitchTemplate) + L"\"";
                }
            if (!effectTemplate.empty())
                {
                bgImgStr += L", \"effect\": \"" + EscapeJsonStr(effectTemplate) + L"\"";
                }
            bgImgStr += L"}";
            }
        else if (!pathTemplate.empty())
            {
            bgImgStr += L"\"image-import\": {\"path\": \"" +
                        EscapeJsonStr(MakeRelativePath(pathTemplate)) + L"\"";
            if (!effectTemplate.empty())
                {
                bgImgStr += L", \"effect\": \"" + EscapeJsonStr(effectTemplate) + L"\"";
                }
            bgImgStr += L"}";
            }

        // size
        const auto widthStr = graph->GetPropertyTemplate(L"size.width");
        const auto heightStr = graph->GetPropertyTemplate(L"size.height");
        if (!widthStr.empty() || !heightStr.empty())
            {
            bgImgStr += L", \"size\": {";
            bool needComma = false;
            if (!widthStr.empty())
                {
                bgImgStr += L"\"width\": " + widthStr;
                needComma = true;
                }
            if (!heightStr.empty())
                {
                if (needComma)
                    {
                    bgImgStr += L", ";
                    }
                bgImgStr += L"\"height\": " + heightStr;
                }
            bgImgStr += L"}";
            }

        // opacity
        const auto opacity = graph->GetPlotBackgroundImageOpacity();
        if (opacity != wxALPHA_OPAQUE)
            {
            bgImgStr += L", \"opacity\": " + std::to_wstring(opacity);
            }

        // image-fit
        const auto imgFit = graph->GetPlotBackgroundImageFit();
        if (imgFit != Wisteria::ImageFit::Shrink)
            {
            const auto fitStr = Wisteria::ReportEnumConvert::ConvertImageFitToString(imgFit);
            if (fitStr.has_value())
                {
                bgImgStr += L", \"image-fit\": \"" + fitStr.value() + L"\"";
                }
            }

        bgImgStr += L"}";
        graphNode->Add(L"background-image", wxSimpleJSON::Create(bgImgStr));
        }

    // stipple-shape (only meaningful when box-effect is stipple-shape)
    if (graph->GetStippleShape() != Wisteria::Icons::IconShape::Square)
        {
        const auto iconStr =
            Wisteria::ReportEnumConvert::ConvertIconToString(graph->GetStippleShape());
        if (iconStr.has_value())
            {
            wxString ssObj = L"{\"icon\": \"" + iconStr.value() + L"\"";
            const auto& ssColor = graph->GetStippleShapeColor();
            if (ssColor.IsOk() &&
                ssColor != Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::White))
                {
                ssObj += L", \"color\": \"" + ColorToStr(ssColor) + L"\"";
                }
            ssObj += L"}";
            graphNode->Add(L"stipple-shape", wxSimpleJSON::Create(ssObj));
            }
        }

    // axes
    const Wisteria::GraphItems::Axis* axes[] = { &graph->GetBottomXAxis(), &graph->GetTopXAxis(),
                                                 &graph->GetLeftYAxis(), &graph->GetRightYAxis() };
    wxString axesStr;
    for (const auto* axis : axes)
        {
        // skip axes that are default/empty (not shown and have no custom settings)
        const auto atStr =
            Wisteria::ReportEnumConvert::ConvertAxisTypeToString(axis->GetAxisType());
        if (!atStr.has_value())
            {
            continue;
            }

        // check if axis has any non-default properties worth writing
        const bool hasTitle = !axis->GetTitle().GetText().empty();
        const auto [rStart, rEnd] = axis->GetRange();
        const bool hasRange = !compare_doubles(rStart, 0.0) || !compare_doubles(rEnd, 0.0);
        const bool hasCustomLabels = !axis->GetCustomLabels().empty();
        const bool hasBrackets = !axis->GetBrackets().empty();
        const bool hasLabelDisplay =
            axis->GetLabelDisplay() != Wisteria::AxisLabelDisplay::DisplayCustomLabelsOrValues;
        const bool hasPenOverride = !axis->GetAxisLinePen().IsOk() ||
                                    axis->GetAxisLinePen() == wxNullPen ||
                                    !(axis->GetAxisLinePen().GetColour() == *wxBLACK &&
                                      axis->GetAxisLinePen().GetWidth() <= 1 &&
                                      axis->GetAxisLinePen().GetStyle() == wxPENSTYLE_SOLID);
        const bool hasGridPenOverride = !axis->GetGridlinePen().IsOk() ||
                                        axis->GetGridlinePen() == wxNullPen ||
                                        !(axis->GetGridlinePen().GetColour() == *wxBLACK &&
                                          axis->GetGridlinePen().GetWidth() <= 1 &&
                                          axis->GetGridlinePen().GetStyle() == wxPENSTYLE_SOLID);
        const bool hasTickOverride =
            axis->GetTickMarkDisplay() != Wisteria::GraphItems::Axis::TickMark::DisplayType::Inner;
        const bool isHidden = !axis->IsShown();

        // skip axes with no meaningful non-default properties
        // (hidden axes with nothing else set, e.g., pie chart axes, are also skipped)
        if (!hasTitle && !hasRange && !hasCustomLabels && !hasBrackets && !hasLabelDisplay)
            {
            continue;
            }

        wxString axisObj = L"{\"axis-type\": \"" + atStr.value() + L"\"";

            // title (always write if axis is being serialized;
            // an empty title overrides the default variable name)
            {
            const auto ttTmpl = axis->GetTitle().GetPropertyTemplate(L"text");
            const auto& ttText = ttTmpl.empty() ? axis->GetTitle().GetText() : ttTmpl;
            axisObj += L", \"title\": {\"text\": \"" + EscapeJsonStr(ttText) + L"\"";
            if (axis->GetTitle().GetTextOrientation() == Wisteria::Orientation::Vertical)
                {
                axisObj += L", \"orientation\": \"vertical\"";
                }
            axisObj += L"}";
            }

        // axis-pen
        if (!axis->GetAxisLinePen().IsOk() || axis->GetAxisLinePen() == wxNullPen)
            {
            axisObj += L", \"axis-pen\": null";
            }
        else if (hasPenOverride)
            {
            axisObj += L", \"axis-pen\": " + SavePenToStr(axis->GetAxisLinePen());
            }

        // gridline-pen
        if (!axis->GetGridlinePen().IsOk() || axis->GetGridlinePen() == wxNullPen)
            {
            axisObj += L", \"gridline-pen\": null";
            }
        else if (hasGridPenOverride)
            {
            axisObj += L", \"gridline-pen\": " + SavePenToStr(axis->GetGridlinePen());
            }

        // label-display
        if (hasLabelDisplay)
            {
            const auto ldStr = Wisteria::ReportEnumConvert::ConvertAxisLabelDisplayToString(
                axis->GetLabelDisplay());
            if (ldStr.has_value())
                {
                axisObj += L", \"label-display\": \"" + ldStr.value() + L"\"";
                }
            }

        // number-display
        if (axis->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
            {
            const auto numDisplayStr =
                Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(axis->GetNumberDisplay());
            if (numDisplayStr.has_value())
                {
                axisObj += L", \"number-display\": \"" + numDisplayStr.value() + L"\"";
                }
            }

        // tickmarks
        if (hasTickOverride)
            {
            const auto tdStr = Wisteria::ReportEnumConvert::ConvertTickMarkDisplayToString(
                axis->GetTickMarkDisplay());
            if (tdStr.has_value())
                {
                axisObj += L", \"tickmarks\": {\"display\": \"" + tdStr.value() + L"\"}";
                }
            }

        // range
        if (hasRange)
            {
            axisObj += wxString::Format(L", \"range\": {\"start\": %g, \"end\": %g", rStart, rEnd);
            if (axis->GetPrecision() != 0)
                {
                axisObj += wxString::Format(L", \"precision\": %d",
                                            static_cast<int>(axis->GetPrecision()));
                }
            if (!compare_doubles(axis->GetInterval(), 0.0))
                {
                axisObj += wxString::Format(L", \"interval\": %g", axis->GetInterval());
                }
            if (axis->GetDisplayInterval() != 1)
                {
                axisObj +=
                    wxString::Format(L", \"display-interval\": %zu", axis->GetDisplayInterval());
                }
            axisObj += L"}";
            }

        // custom-labels
        if (hasCustomLabels)
            {
            axisObj += L", \"custom-labels\": [";
            bool first = true;
            for (const auto& [value, label] : axis->GetCustomLabels())
                {
                if (!first)
                    {
                    axisObj += L", ";
                    }
                first = false;
                const auto ltTmpl = label.GetPropertyTemplate(L"text");
                const auto& ltText = ltTmpl.empty() ? label.GetText() : ltTmpl;
                axisObj += wxString::Format(L"{\"value\": %g, \"label\": \"%s\"}", value,
                                            EscapeJsonStr(ltText));
                }
            axisObj += L"]";
            }

        // brackets
        if (hasBrackets)
            {
            const auto& brackets = axis->GetBrackets();
            const auto bracketDsName = axis->GetPropertyTemplate(L"brackets.dataset");
            if (!bracketDsName.empty())
                {
                axisObj +=
                    L", \"brackets\": {\"dataset\": \"" + EscapeJsonStr(bracketDsName) + L"\"";
                const auto labelVar = axis->GetPropertyTemplate(L"bracket.label");
                const auto valueVar = axis->GetPropertyTemplate(L"bracket.value");
                if (!labelVar.empty() || !valueVar.empty())
                    {
                    axisObj += L", \"variables\": {";
                    bool needComma = false;
                    if (!labelVar.empty())
                        {
                        axisObj += L"\"label\": \"" + EscapeJsonStr(labelVar) + L"\"";
                        needComma = true;
                        }
                    if (!valueVar.empty())
                        {
                        if (needComma)
                            {
                            axisObj += L", ";
                            }
                        axisObj += L"\"value\": \"" + EscapeJsonStr(valueVar) + L"\"";
                        }
                    axisObj += L"}";
                    }
                axisObj += L"}";
                }
            else
                {
                axisObj += L", \"brackets\": [";
                for (size_t i = 0; i < brackets.size(); ++i)
                    {
                    if (i > 0)
                        {
                        axisObj += L", ";
                        }
                    const auto& b = brackets[i];
                    axisObj += wxString::Format(L"{\"start\": %g, \"end\": %g",
                                                b.GetStartPosition(), b.GetEndPosition());
                    if (!b.GetLabel().GetText().empty())
                        {
                        axisObj +=
                            L", \"label\": \"" + EscapeJsonStr(b.GetLabel().GetText()) + L"\"";
                        }
                    const auto bsStr = Wisteria::ReportEnumConvert::ConvertBracketLineStyleToString(
                        b.GetBracketLineStyle());
                    if (bsStr.has_value() &&
                        b.GetBracketLineStyle() != Wisteria::BracketLineStyle::CurlyBraces)
                        {
                        axisObj += L", \"style\": \"" + bsStr.value() + L"\"";
                        }
                    axisObj += L"}";
                    }
                axisObj += L"]";
                }
            }

        // show (default is true)
        if (isHidden)
            {
            axisObj += L", \"show\": false";
            }

        axisObj += L"}";

        if (!axesStr.empty())
            {
            axesStr += L", ";
            }
        axesStr += axisObj;
        }

    // reference-lines
    const auto& refLines = graph->GetReferenceLines();
    wxString refLinesStr;
    for (const auto& rl : refLines)
        {
        const auto atStr = Wisteria::ReportEnumConvert::ConvertAxisTypeToString(rl.GetAxisType());
        if (!atStr.has_value())
            {
            continue;
            }
        wxString rlObj = L"{\"axis-type\": \"" + atStr.value() + L"\"";
        rlObj += wxString::Format(L", \"position\": %g", rl.GetAxisPosition());
        if (!rl.GetLabel().empty())
            {
            rlObj += L", \"label\": \"" + EscapeJsonStr(rl.GetLabel()) + L"\"";
            }
        const auto& rlPen = rl.GetPen();
        if (rlPen.IsOk() && rlPen != wxNullPen)
            {
            rlObj += L", \"pen\": " + SavePenToStr(rlPen);
            }
        if (rl.GetLabelPlacement() != Wisteria::ReferenceLabelPlacement::Legend)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertReferenceLabelPlacementToString(
                rl.GetLabelPlacement());
            if (lpStr.has_value())
                {
                rlObj += L", \"reference-label-placement\": \"" + lpStr.value() + L"\"";
                }
            }
        rlObj += L"}";
        if (!refLinesStr.empty())
            {
            refLinesStr += L", ";
            }
        refLinesStr += rlObj;
        }

    // reference-areas
    const auto& refAreas = graph->GetReferenceAreas();
    wxString refAreasStr;
    for (const auto& ra : refAreas)
        {
        const auto atStr = Wisteria::ReportEnumConvert::ConvertAxisTypeToString(ra.GetAxisType());
        if (!atStr.has_value())
            {
            continue;
            }
        wxString raObj = L"{\"axis-type\": \"" + atStr.value() + L"\"";
        raObj += wxString::Format(L", \"start\": %g, \"end\": %g", ra.GetAxisPosition(),
                                  ra.GetAxisPosition2());
        if (!ra.GetLabel().empty())
            {
            raObj += L", \"label\": \"" + EscapeJsonStr(ra.GetLabel()) + L"\"";
            }
        const auto& raPen = ra.GetPen();
        if (raPen.IsOk() && raPen != wxNullPen)
            {
            raObj += L", \"pen\": " + SavePenToStr(raPen);
            }
        if (ra.GetReferenceAreaStyle() != Wisteria::ReferenceAreaStyle::Solid)
            {
            const auto rasStr = Wisteria::ReportEnumConvert::ConvertReferenceAreaStyleToString(
                ra.GetReferenceAreaStyle());
            if (rasStr.has_value())
                {
                raObj += L", \"style\": \"" + rasStr.value() + L"\"";
                }
            }
        raObj += L"}";
        if (!refAreasStr.empty())
            {
            refAreasStr += L", ";
            }
        refAreasStr += raObj;
        }

    // add sub-objects directly to the node
    if (!datasetName.empty())
        {
        graphNode->Add(L"dataset", datasetName);
        }
    if (!varsStr.empty())
        {
        graphNode->Add(L"variables", wxSimpleJSON::Create(L"{" + varsStr + L"}"));
        }
    if (!titleStr.empty())
        {
        graphNode->Add(L"title", wxSimpleJSON::Create(titleStr));
        }
    if (!subtitleStr.empty())
        {
        graphNode->Add(L"sub-title", wxSimpleJSON::Create(subtitleStr));
        }
    if (!captionStr.empty())
        {
        if (!graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)) ||
            graph->GetPropertyTemplate(L"footnotes").empty())
            {
            graphNode->Add(L"caption", wxSimpleJSON::Create(captionStr));
            }
        }
    if (!axesStr.empty())
        {
        graphNode->Add(L"axes", wxSimpleJSON::Create(L"[" + axesStr + L"]"));
        }
    if (!refLinesStr.empty())
        {
        graphNode->Add(L"reference-lines", wxSimpleJSON::Create(L"[" + refLinesStr + L"]"));
        }
    if (!refAreasStr.empty())
        {
        graphNode->Add(L"reference-areas", wxSimpleJSON::Create(L"[" + refAreasStr + L"]"));
        }

    // annotations
    const auto& annotations = graph->GetAnnotations();
    if (!annotations.empty())
        {
        wxString annotationsStr;
        for (const auto& ann : annotations)
            {
            const auto* label =
                dynamic_cast<const Wisteria::GraphItems::Label*>(ann.GetObject().get());
            if (label == nullptr)
                {
                continue;
                }
            wxString annObj = L"{\"label\": " + SaveLabelPropertiesToStr(*label);
            // anchor point
            const auto anchor = ann.GetAnchorPoint();
            annObj +=
                wxString::Format(L", \"anchor\": {\"x\": %g, \"y\": %g}", anchor.m_x, anchor.m_y);
            // interest points
            const auto& interestPts = ann.GetInterestPoints();
            if (!interestPts.empty())
                {
                wxString ptsStr;
                for (const auto& pt : interestPts)
                    {
                    if (!ptsStr.empty())
                        {
                        ptsStr += L", ";
                        }
                    ptsStr += wxString::Format(L"{\"x\": %g, \"y\": %g}", pt.m_x, pt.m_y);
                    }
                annObj += L", \"interest-points\": [" + ptsStr + L"]";
                }
            annObj += L"}";
            if (!annotationsStr.empty())
                {
                annotationsStr += L", ";
                }
            annotationsStr += annObj;
            }
        if (!annotationsStr.empty())
            {
            graphNode->Add(L"annotations", wxSimpleJSON::Create(L"[" + annotationsStr + L"]"));
            }
        }

    // legend
    const auto& legendInfo = graph->GetLegendInfo();
    if (legendInfo.has_value())
        {
        wxString legendObj = L"{";
        const auto placement = legendInfo->GetPlacement();
        if (placement == Wisteria::Side::Left)
            {
            legendObj += L"\"placement\": \"left\"";
            }
        else if (placement == Wisteria::Side::Top)
            {
            legendObj += L"\"placement\": \"top\"";
            }
        else if (placement == Wisteria::Side::Bottom)
            {
            legendObj += L"\"placement\": \"bottom\"";
            }
        else
            {
            legendObj += L"\"placement\": \"right\"";
            }
        if (!legendInfo->IsIncludingHeader())
            {
            legendObj += L", \"include-header\": false";
            }
        if (legendInfo->GetRingPerimeter() == Wisteria::Perimeter::Inner)
            {
            legendObj += L", \"ring\": \"inner\"";
            }
        if (!legendInfo->GetTitle().empty())
            {
            legendObj += L", \"title\": \"" + EscapeJsonStr(legendInfo->GetTitle()) + L"\"";
            }
        legendObj += L"}";
        graphNode->Add(L"legend", wxSimpleJSON::Create(legendObj));
        }

    // pen (item-level pen for the graph)
    const auto& graphPen = graph->GetPen();
    if (!graphPen.IsOk() || graphPen == wxNullPen)
        {
        graphNode->AddNull(L"pen");
        }
    else if (!(graphPen.GetColour() == *wxBLACK && graphPen.GetWidth() <= 1 &&
               graphPen.GetStyle() == wxPENSTYLE_SOLID))
        {
        graphNode->Add(L"pen", wxSimpleJSON::Create(SavePenToStr(graphPen)));
        }

    // brush-scheme
    if (graph->GetBrushScheme() != nullptr && !graph->GetBrushScheme()->GetBrushes().empty())
        {
        const auto& brushes = graph->GetBrushScheme()->GetBrushes();
        bool allSolid = true;
        for (const auto& br : brushes)
            {
            if (br.GetStyle() != wxBRUSHSTYLE_SOLID)
                {
                allSolid = false;
                break;
                }
            }

        // check if the color scheme is a named scheme —
        // if so, save by name instead of enumerating colors
        const auto& cs = graph->GetColorScheme();
        const int csIndex =
            (cs != nullptr) ? Wisteria::UI::InsertGraphDlg::ColorSchemeToIndex(cs) : 0;

        if (allSolid && csIndex > 0)
            {
            // named color scheme with solid brushes — save by name
            const auto schemeName = Wisteria::UI::InsertGraphDlg::ColorSchemeToName(csIndex);
            graphNode->Add(L"brush-scheme", schemeName);
            graphNode->Add(L"color-scheme", schemeName);
            }
        else
            {
            wxString colorsArr = L"[";
            for (size_t i = 0; i < brushes.size(); ++i)
                {
                if (i > 0)
                    {
                    colorsArr += L", ";
                    }
                colorsArr += L"\"" + ColorToStr(brushes[i].GetColour()) + L"\"";
                }
            colorsArr += L"]";

            if (allSolid)
                {
                graphNode->Add(L"brush-scheme",
                               wxSimpleJSON::Create(L"{\"color-scheme\": " + colorsArr + L"}"));
                }
            else
                {
                wxString stylesArr = L"[";
                for (size_t i = 0; i < brushes.size(); ++i)
                    {
                    if (i > 0)
                        {
                        stylesArr += L", ";
                        }
                    const auto bsStr = Wisteria::ReportEnumConvert::ConvertBrushStyleToString(
                        brushes[i].GetStyle());
                    stylesArr +=
                        L"\"" + (bsStr.has_value() ? bsStr.value() : wxString(L"solid")) + L"\"";
                    }
                stylesArr += L"]";
                graphNode->Add(L"brush-scheme",
                               wxSimpleJSON::Create(L"{\"brush-styles\": " + stylesArr +
                                                    L", \"color-scheme\": " + colorsArr + L"}"));
                }
            }
        }

    // color-scheme — save named schemes by name, otherwise enumerate individual colors
    // (only if no brush-scheme was written, since brush-scheme embeds its own)
    if ((graph->GetBrushScheme() == nullptr || graph->GetBrushScheme()->GetBrushes().empty()) &&
        graph->GetColorScheme() != nullptr && !graph->GetColorScheme()->GetColors().empty())
        {
        // clang-format off
        const auto& cs = graph->GetColorScheme();
        if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::ArcticChill)))
            { graphNode->Add(L"color-scheme", wxString{ L"arcticchill" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::BackToSchool)))
            { graphNode->Add(L"color-scheme", wxString{ L"backtoschool" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::BoxOfChocolates)))
            { graphNode->Add(L"color-scheme", wxString{ L"boxofchocolates" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Campfire)))
            { graphNode->Add(L"color-scheme", wxString{ L"campfire" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::CoffeeShop)))
            { graphNode->Add(L"color-scheme", wxString{ L"coffeeshop" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Cosmopolitan)))
            { graphNode->Add(L"color-scheme", wxString{ L"cosmopolitan" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::DayAndNight)))
            { graphNode->Add(L"color-scheme", wxString{ L"dayandnight" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1920s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1920s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1940s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1940s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1950s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1950s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1960s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1960s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1970s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1970s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1980s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1980s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade1990s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade1990s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Decade2000s)))
            { graphNode->Add(L"color-scheme", wxString{ L"decade2000s" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Dusk)))
            { graphNode->Add(L"color-scheme", wxString{ L"dusk" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::EarthTones)))
            { graphNode->Add(L"color-scheme", wxString{ L"earthtones" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::FreshFlowers)))
            { graphNode->Add(L"color-scheme", wxString{ L"freshflowers" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::IceCream)))
            { graphNode->Add(L"color-scheme", wxString{ L"icecream" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::MeadowSunset)))
            { graphNode->Add(L"color-scheme", wxString{ L"meadowsunset" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Nautical)))
            { graphNode->Add(L"color-scheme", wxString{ L"nautical" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::October)))
            { graphNode->Add(L"color-scheme", wxString{ L"october" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::ProduceSection)))
            { graphNode->Add(L"color-scheme", wxString{ L"producesection" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::RollingThunder)))
            { graphNode->Add(L"color-scheme", wxString{ L"rollingthunder" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Seasons)))
            { graphNode->Add(L"color-scheme", wxString{ L"seasons" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Semesters)))
            { graphNode->Add(L"color-scheme", wxString{ L"semesters" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::ShabbyChic)))
            { graphNode->Add(L"color-scheme", wxString{ L"shabbychic" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Slytherin)))
            { graphNode->Add(L"color-scheme", wxString{ L"slytherin" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Spring)))
            { graphNode->Add(L"color-scheme", wxString{ L"spring" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::TastyWaves)))
            { graphNode->Add(L"color-scheme", wxString{ L"tastywaves" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::Typewriter)))
            { graphNode->Add(L"color-scheme", wxString{ L"typewriter" }); }
        else if (cs->IsKindOf(wxCLASSINFO(Wisteria::Colors::Schemes::UrbanOasis)))
            { graphNode->Add(L"color-scheme", wxString{ L"urbanoasis" }); }
        else
            {
            // unrecognized scheme — fall back to enumerating individual colors
            wxString colorsArr = L"[";
            const auto& colors = cs->GetColors();
            for (size_t i = 0; i < colors.size(); ++i)
                {
                if (i > 0)
                    {
                    colorsArr += L", ";
                    }
                colorsArr += L"\"" + ColorToStr(colors[i]) + L"\"";
                }
            colorsArr += L"]";
            graphNode->Add(L"color-scheme", wxSimpleJSON::Create(colorsArr));
            }
        // clang-format on
        }

    // icon-scheme — save named schemes by name, otherwise enumerate individual icons
    if (graph->GetShapeScheme() != nullptr && !graph->GetShapeScheme()->GetShapes().empty())
        {
        if (graph->GetShapeScheme()->IsKindOf(wxCLASSINFO(Wisteria::Icons::Schemes::Semesters)))
            {
            graphNode->Add(L"icon-scheme", wxString{ L"semesters" });
            }
        else if (!graph->GetShapeScheme()->IsKindOf(
                     wxCLASSINFO(Wisteria::Icons::Schemes::StandardShapes)))
            {
            wxString iconsArr = L"[";
            const auto& shapes = graph->GetShapeScheme()->GetShapes();
            for (size_t i = 0; i < shapes.size(); ++i)
                {
                if (i > 0)
                    {
                    iconsArr += L", ";
                    }
                const auto iconStr = Wisteria::ReportEnumConvert::ConvertIconToString(shapes[i]);
                iconsArr += L"\"" +
                            (iconStr.has_value() ? iconStr.value() : wxString(L"blank-icon")) +
                            L"\"";
                }
            iconsArr += L"]";
            graphNode->Add(L"icon-scheme", wxSimpleJSON::Create(iconsArr));
            }
        }

    SaveItem(graphNode, graph, canvas);
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SaveGraphByType(const Wisteria::Graphs::Graph2D* graph,
                                                 const Wisteria::Canvas* canvas) const
    {
    if (graph == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    const auto typeStr = WisteriaApp::GetGraphTypeString(graph);
    if (typeStr.empty())
        {
        return wxSimpleJSON::Ptr_t{};
        }

    auto node =
        wxSimpleJSON::Create(L"{\"type\": \"" + typeStr +
                             L"\", \"canvas-margins\": [], \"padding\": [], \"outline\": []}");

    SaveGraph(graph, node, canvas);

    // type-specific options
    if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LinePlot)))
        {
        const auto* linePlot = dynamic_cast<const Wisteria::Graphs::LinePlot*>(graph);
        // LinePlot default ghost opacity is max(75, GHOST_OPACITY)
        if (linePlot->GetGhostOpacity() != std::max<uint8_t>(75, Wisteria::Settings::GHOST_OPACITY))
            {
            node->Add(L"ghost-opacity", static_cast<double>(linePlot->GetGhostOpacity()));
            }
        // showcase-lines (cached as indexed templates: showcase-lines[0], [1], ...)
        wxString showcaseArr;
        for (size_t i = 0;; ++i)
            {
            const auto val =
                graph->GetPropertyTemplate(L"showcase-lines[" + std::to_wstring(i) + L"]");
            if (val.empty())
                {
                break;
                }
            if (!showcaseArr.empty())
                {
                showcaseArr += L", ";
                }
            showcaseArr += L"\"" + EscapeJsonStr(val) + L"\"";
            }
        if (!showcaseArr.empty())
            {
            node->Add(L"showcase-lines", wxSimpleJSON::Create(L"[" + showcaseArr + L"]"));
            }
        // line-scheme
        if (linePlot->GetLineStyleScheme() != nullptr &&
            !linePlot->GetLineStyleScheme()->GetLineStyles().empty())
            {
            const auto& lineStyles = linePlot->GetLineStyleScheme()->GetLineStyles();
            // skip if it's just the default (single solid+lines entry)
            const bool isDefault =
                (lineStyles.size() == 1 && lineStyles.front().first == wxPENSTYLE_SOLID &&
                 lineStyles.front().second == Wisteria::LineStyle::Lines);
            if (!isDefault)
                {
                wxString lsArr = L"[";
                for (size_t i = 0; i < lineStyles.size(); ++i)
                    {
                    if (i > 0)
                        {
                        lsArr += L", ";
                        }
                    lsArr += L"{";
                    const auto psStr =
                        Wisteria::ReportEnumConvert::ConvertPenStyleToString(lineStyles[i].first);
                    if (psStr.has_value() && lineStyles[i].first != wxPENSTYLE_SOLID)
                        {
                        lsArr += L"\"pen-style\": {\"style\": \"" + psStr.value() + L"\"}, ";
                        }
                    const auto lStr =
                        Wisteria::ReportEnumConvert::ConvertLineStyleToString(lineStyles[i].second);
                    lsArr += L"\"line-style\": \"" +
                             (lStr.has_value() ? lStr.value() : wxString(L"lines")) + L"\"}";
                    }
                lsArr += L"]";
                node->Add(L"line-scheme", wxSimpleJSON::Create(lsArr));
                }
            }
        // w-curve-plot specific
        if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WCurvePlot)))
            {
            const auto* wcPlot = dynamic_cast<const Wisteria::Graphs::WCurvePlot*>(graph);
            node->Add(L"time-interval-label", wcPlot->GetTimeIntervalLabel());
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScatterPlot)))
        {
        const auto* scatterPlot = dynamic_cast<const Wisteria::Graphs::ScatterPlot*>(graph);
        if (scatterPlot->IsShowingRegressionLines())
            {
            node->Add(L"show-regression-lines", true);
            }
        if (scatterPlot->IsShowingConfidenceBands())
            {
            node->Add(L"show-confidence-bands", true);
            }
        if (!compare_doubles(scatterPlot->GetConfidenceLevel(), 0.95))
            {
            node->Add(L"confidence-level", scatterPlot->GetConfidenceLevel());
            }
        // regression-line-scheme
        if (scatterPlot->GetRegressionLineStyleScheme() != nullptr &&
            !scatterPlot->GetRegressionLineStyleScheme()->GetLineStyles().empty())
            {
            const auto& lineStyles = scatterPlot->GetRegressionLineStyleScheme()->GetLineStyles();
            const bool isDefault =
                (lineStyles.size() == 1 && lineStyles.front().first == wxPENSTYLE_SOLID &&
                 lineStyles.front().second == Wisteria::LineStyle::Lines);
            if (!isDefault)
                {
                wxString lsArr = L"[";
                for (size_t i = 0; i < lineStyles.size(); ++i)
                    {
                    if (i > 0)
                        {
                        lsArr += L", ";
                        }
                    lsArr += L"{";
                    const auto psStr =
                        Wisteria::ReportEnumConvert::ConvertPenStyleToString(lineStyles[i].first);
                    if (psStr.has_value() && lineStyles[i].first != wxPENSTYLE_SOLID)
                        {
                        lsArr += L"\"pen-style\": {\"style\": \"" + psStr.value() + L"\"}, ";
                        }
                    const auto lStr =
                        Wisteria::ReportEnumConvert::ConvertLineStyleToString(lineStyles[i].second);
                    lsArr += L"\"line-style\": \"" +
                             (lStr.has_value() ? lStr.value() : wxString(L"lines")) + L"\"}";
                    }
                lsArr += L"]";
                node->Add(L"regression-line-scheme", wxSimpleJSON::Create(lsArr));
                }
            }
        // bubble-plot specific
        if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BubblePlot)))
            {
            const auto* bubblePlot = dynamic_cast<const Wisteria::Graphs::BubblePlot*>(graph);
            if (bubblePlot->GetMinBubbleRadius() != 4)
                {
                node->Add(L"min-bubble-radius",
                          static_cast<double>(bubblePlot->GetMinBubbleRadius()));
                }
            if (bubblePlot->GetMaxBubbleRadius() != 30)
                {
                node->Add(L"max-bubble-radius",
                          static_cast<double>(bubblePlot->GetMaxBubbleRadius()));
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LikertChart)))
        {
        const auto* likert = dynamic_cast<const Wisteria::Graphs::LikertChart*>(graph);
        // survey-format
        const auto sfStr = Wisteria::ReportEnumConvert::ConvertLikertSurveyQuestionFormatToString(
            likert->GetSurveyType());
        if (sfStr.has_value())
            {
            node->Add(L"survey-format", sfStr.value());
            }
        // colors (only if non-default)
        if (likert->GetNegativeColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Orange))
            {
            node->Add(L"negative-color", ColorToStr(likert->GetNegativeColor()));
            }
        if (likert->GetPositiveColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Cerulean))
            {
            node->Add(L"positive-color", ColorToStr(likert->GetPositiveColor()));
            }
        if (likert->GetNeutralColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::AshGrey))
            {
            node->Add(L"neutral-color", ColorToStr(likert->GetNeutralColor()));
            }
        if (likert->GetNoResponseColor() !=
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::White))
            {
            node->Add(L"no-response-color", ColorToStr(likert->GetNoResponseColor()));
            }
        // boolean options
        if (likert->IsShowingResponseCounts())
            {
            node->Add(L"show-response-counts", true);
            }
        if (!likert->IsShowingPercentages())
            {
            node->Add(L"show-percentages", false);
            }
        if (!likert->IsShowingSectionHeaders())
            {
            node->Add(L"show-section-headers", false);
            }
        if (likert->IsSettingBarSizesToRespondentSize())
            {
            node->Add(L"adjust-bar-widths-to-respondent-size", true);
            }
        // header labels
        node->Add(L"positive-label", likert->GetPositiveHeader());
        node->Add(L"negative-label", likert->GetNegativeHeader());
        node->Add(L"no-response-label", likert->GetNoResponseHeader());
        // question-brackets
        const auto& brackets = likert->GetQuestionsBrackets();
        if (!brackets.empty())
            {
            wxString bArr = L"[";
            for (size_t i = 0; i < brackets.size(); ++i)
                {
                if (i > 0)
                    {
                    bArr += L", ";
                    }
                bArr += L"{\"start\": \"" + EscapeJsonStr(brackets[i].m_question1) +
                        L"\", \"end\": \"" + EscapeJsonStr(brackets[i].m_question2) +
                        L"\", \"title\": \"" + EscapeJsonStr(brackets[i].m_title) + L"\"}";
                }
            bArr += L"]";
            node->Add(L"question-brackets", wxSimpleJSON::Create(bArr));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ScaleChart)))
        {
        const auto* scaleChart = dynamic_cast<const Wisteria::Graphs::ScaleChart*>(graph);
        if (scaleChart->IsShowcasingScore())
            {
            node->Add(L"showcase-score", true);
            }
        // main scale values and precision
        if (!scaleChart->GetMainScaleValues().empty())
            {
            wxString scaleValsArr = L"[";
            for (const auto val : scaleChart->GetMainScaleValues())
                {
                if (scaleValsArr.length() > 1)
                    {
                    scaleValsArr += L", ";
                    }
                scaleValsArr += std::to_wstring(val);
                }
            scaleValsArr += L"]";
            node->Add(L"main-scale-values", wxSimpleJSON::Create(scaleValsArr));
            node->Add(L"main-scale-precision",
                      static_cast<double>(scaleChart->GetMainScalePrecision()));
            }
        // main-scale and data column headers (from opposite bar axis)
        const auto& bars = scaleChart->GetBars();
        if (!bars.empty())
            {
            const auto& mainHeader =
                scaleChart->GetOppositeBarAxis().GetCustomLabel(bars[0].GetAxisPosition());
            const auto mainHeaderTmpl = mainHeader.GetPropertyTemplate(L"text");
            const auto& mainHeaderText =
                mainHeaderTmpl.empty() ? mainHeader.GetText() : mainHeaderTmpl;
            if (!mainHeaderText.empty())
                {
                node->Add(L"main-scale-header", mainHeaderText);
                }
            if (bars.size() > 1)
                {
                const auto& dataHeader =
                    scaleChart->GetOppositeBarAxis().GetCustomLabel(bars[1].GetAxisPosition());
                const auto dataHeaderTmpl = dataHeader.GetPropertyTemplate(L"text");
                const auto& dataHeaderText =
                    dataHeaderTmpl.empty() ? dataHeader.GetText() : dataHeaderTmpl;
                if (!dataHeaderText.empty())
                    {
                    node->Add(L"data-column-header", dataHeaderText);
                    }
                }
            }
        // scales (skip first two placeholder bars)
        if (bars.size() > 2)
            {
            wxString scalesArr = L"[";
            for (size_t barIdx = 2; barIdx < bars.size(); ++barIdx)
                {
                if (scalesArr.length() > 1)
                    {
                    scalesArr += L", ";
                    }
                const auto& scaleBar = bars[barIdx];
                const auto& scaleHeader =
                    scaleChart->GetOppositeBarAxis().GetCustomLabel(scaleBar.GetAxisPosition());
                const auto scaleHeaderTmpl = scaleHeader.GetPropertyTemplate(L"text");
                const auto& scaleHeaderText =
                    scaleHeaderTmpl.empty() ? scaleHeader.GetText() : scaleHeaderTmpl;

                scalesArr += L"{";
                if (!scaleHeaderText.empty())
                    {
                    scalesArr += L"\"header\": \"" + EscapeJsonStr(scaleHeaderText) + L"\"";
                    }
                if (scaleBar.GetCustomScalingAxisStartPosition().has_value())
                    {
                    if (!scaleHeaderText.empty())
                        {
                        scalesArr += L", ";
                        }
                    scalesArr +=
                        L"\"start\": " +
                        std::to_wstring(scaleBar.GetCustomScalingAxisStartPosition().value());
                    }

                wxString blocksArr = L"[";
                for (const auto& block : scaleBar.GetBlocks())
                    {
                    if (blocksArr.length() > 1)
                        {
                        blocksArr += L", ";
                        }
                    const auto labelTmpl = block.GetDecal().GetPropertyTemplate(L"text");
                    const auto& labelText =
                        labelTmpl.empty() ? block.GetDecal().GetText() : labelTmpl;

                    blocksArr += L"{\"length\": " + std::to_wstring(block.GetLength()) +
                                 L", \"color\": \"" + ColorToStr(block.GetBrush().GetColour()) +
                                 L"\"";
                    if (!labelText.empty())
                        {
                        blocksArr += L", \"label\": \"" + EscapeJsonStr(labelText) + L"\"";
                        }
                    blocksArr += L"}";
                    }
                blocksArr += L"]";
                if (!scaleHeaderText.empty() ||
                    scaleBar.GetCustomScalingAxisStartPosition().has_value())
                    {
                    scalesArr += L", ";
                    }
                scalesArr += L"\"blocks\": " + blocksArr + L"}";
                }
            scalesArr += L"]";
            node->Add(L"scales", wxSimpleJSON::Create(scalesArr));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BarChart)))
        {
        const auto* barChart = dynamic_cast<const Wisteria::Graphs::BarChart*>(graph);
        // shared BarChart options
        const auto beStr =
            Wisteria::ReportEnumConvert::ConvertBoxEffectToString(barChart->GetBarEffect());
        if (beStr.has_value() && barChart->GetBarEffect() != Wisteria::BoxEffect::Solid)
            {
            node->Add(L"box-effect", beStr.value());
            }
        if (barChart->GetBarOrientation() == Wisteria::Orientation::Vertical)
            {
            node->Add(L"bar-orientation", wxString{ _DT(L"vertical") });
            }
        if (barChart->GetNumberDisplay() != Wisteria::NumberDisplay::Value)
            {
            const auto numDisplayStr = Wisteria::ReportEnumConvert::ConvertNumberDisplayToString(
                barChart->GetNumberDisplay());
            if (numDisplayStr.has_value())
                {
                node->Add(L"number-display", numDisplayStr.value());
                }
            }
        if (barChart->GetBinLabelDisplay() != Wisteria::BinLabelDisplay::BinValue)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                barChart->GetBinLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"bar-label-display", blStr.value());
                }
            }
        if (barChart->GetBarGroupPlacement() != Wisteria::LabelPlacement::NextToParent)
            {
            const auto bgStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                barChart->GetBarGroupPlacement());
            if (bgStr.has_value())
                {
                node->Add(L"bar-group-placement", bgStr.value());
                }
            }
        // bar-shapes: single string if all bars share a non-default shape;
        // otherwise a per-bar array keyed by axis label (Rectangle bars omitted)
        if (!barChart->GetBars().empty())
            {
            const auto firstShape = barChart->GetBars().front().GetShape();
            bool allSame = true;
            for (const auto& bar : barChart->GetBars())
                {
                if (bar.GetShape() != firstShape)
                    {
                    allSame = false;
                    break;
                    }
                }
            if (allSame && firstShape != Wisteria::Graphs::BarChart::BarShape::Rectangle)
                {
                const auto bsStr = Wisteria::ReportEnumConvert::ConvertBarShapeToString(firstShape);
                if (bsStr.has_value())
                    {
                    node->Add(L"bar-shapes", bsStr.value());
                    }
                }
            else if (!allSame)
                {
                wxString shapesArr;
                for (const auto& bar : barChart->GetBars())
                    {
                    if (bar.GetShape() == Wisteria::Graphs::BarChart::BarShape::Rectangle)
                        {
                        continue;
                        }
                    const auto bsStr =
                        Wisteria::ReportEnumConvert::ConvertBarShapeToString(bar.GetShape());
                    if (!bsStr.has_value())
                        {
                        continue;
                        }
                    if (!shapesArr.empty())
                        {
                        shapesArr += L", ";
                        }
                    shapesArr += L"{\"axis-label\": \"" +
                                 EscapeJsonStr(bar.GetAxisLabel().GetText()) +
                                 L"\", \"shape\": \"" + bsStr.value() + L"\"}";
                    }
                if (!shapesArr.empty())
                    {
                    node->Add(L"bar-shapes", wxSimpleJSON::Create(L"[" + shapesArr + L"]"));
                    }
                }
            }
        if (barChart->GetGhostOpacity() != Wisteria::Settings::GHOST_OPACITY)
            {
            node->Add(L"ghost-opacity", static_cast<double>(barChart->GetGhostOpacity()));
            }
        // showcase-bars (indexed templates)
        wxString showcaseBarArr;
        for (size_t i = 0;; ++i)
            {
            const auto val =
                graph->GetPropertyTemplate(L"showcase-bars[" + std::to_wstring(i) + L"]");
            if (val.empty())
                {
                break;
                }
            if (!showcaseBarArr.empty())
                {
                showcaseBarArr += L", ";
                }
            showcaseBarArr += L"\"" + EscapeJsonStr(val) + L"\"";
            }
        if (!showcaseBarArr.empty())
            {
            node->Add(L"showcase-bars", wxSimpleJSON::Create(L"[" + showcaseBarArr + L"]"));
            }

        // decals
        wxString decalsArr;
        for (const auto& bar : barChart->GetBars())
            {
            for (size_t bk = 0; bk < bar.GetBlocks().size(); ++bk)
                {
                const auto& decal = bar.GetBlocks()[bk].GetDecal();
                if (decal.GetText().empty())
                    {
                    continue;
                    }
                if (!decalsArr.empty())
                    {
                    decalsArr += L", ";
                    }
                const auto textTmpl = decal.GetPropertyTemplate(L"text");
                const auto& decalText = textTmpl.empty() ? decal.GetText() : textTmpl;
                decalsArr += L"{\"bar\": \"" + EscapeJsonStr(bar.GetAxisLabel().GetText()) + L"\"";
                if (bk != 0)
                    {
                    decalsArr += L", \"block\": " + std::to_wstring(bk);
                    }
                decalsArr += L", \"decal\": {\"text\": \"" + EscapeJsonStr(decalText) + L"\"";
                const auto& decalColor = decal.GetFontColor();
                if (decalColor.IsOk() && decalColor != *wxBLACK)
                    {
                    decalsArr += L", \"color\": \"" + ColorToStr(decalColor) + L"\"";
                    }
                const auto& decalBgColor = decal.GetFontBackgroundColor();
                if (decalBgColor.IsOk())
                    {
                    decalsArr += L", \"background\": \"" + ColorToStr(decalBgColor) + L"\"";
                    }
                decalsArr += L"}}";
                }
            }
        if (!decalsArr.empty())
            {
            node->Add(L"decals", wxSimpleJSON::Create(L"[" + decalsArr + L"]"));
            }

        if (!barChart->GetBinLabelSuffix().empty())
            {
            node->Add(L"bar-label-suffix", barChart->GetBinLabelSuffix());
            }
        if (barChart->IsIncludingSpacesBetweenBars())
            {
            node->Add(L"include-spaces-between-bars", true);
            }
        if (barChart->IsConstrainingScalingAxisToBars())
            {
            node->Add(L"constrain-scaling-axis-to-bars", true);
            }
        if (barChart->IsApplyingBrushesToUngroupedBars())
            {
            node->Add(L"apply-brushes-to-ungrouped-bars", true);
            }
        if (!barChart->IsHidingGhostedLabels())
            {
            node->Add(L"hide-ghosted-labels", false);
            }
        // bar-sort (only if explicitly set via JSON, not from SetData's internal sort)
        const auto barSortTemplate = graph->GetPropertyTemplate(L"bar-sort");
        if (!barSortTemplate.empty())
            {
            wxString sortObj = L"{\"direction\": \"";
            sortObj += (barChart->GetSortDirection() == Wisteria::SortDirection::SortAscending) ?
                           L"ascending" :
                           L"descending";
            sortObj += L"\"";
            if (barChart->GetSortComparison().has_value())
                {
                sortObj += L", \"by\": \"";
                sortObj += (barChart->GetSortComparison().value() ==
                            Wisteria::Graphs::BarChart::BarSortComparison::SortByBarLength) ?
                               L"length" :
                               L"label";
                sortObj += L"\"";
                }
            else if (!barChart->GetSortLabels().empty())
                {
                sortObj += L", \"labels\": [";
                for (size_t i = 0; i < barChart->GetSortLabels().size(); ++i)
                    {
                    if (i > 0)
                        {
                        sortObj += L", ";
                        }
                    sortObj += L"\"" + EscapeJsonStr(barChart->GetSortLabels()[i]) + L"\"";
                    }
                sortObj += L"]";
                }
            sortObj += L"}";
            node->Add(L"bar-sort", wxSimpleJSON::Create(sortObj));
            }
        // bar-groups
        if (!barChart->GetBarGroups().empty())
            {
            wxString bgArr = L"[";
            for (size_t i = 0; i < barChart->GetBarGroups().size(); ++i)
                {
                const auto& bg = barChart->GetBarGroups()[i];
                if (i > 0)
                    {
                    bgArr += L", ";
                    }
                bgArr += L"{";
                bool hasField = false;
                if (bg.m_barColor.IsOk())
                    {
                    bgArr += L"\"color\": \"" + ColorToStr(bg.m_barColor) + L"\"";
                    hasField = true;
                    }
                if (bg.m_barBrush.IsOk() && bg.m_barBrush != wxNullBrush)
                    {
                    bgArr += wxString(hasField ? L", " : L"") + L"\"brush\": " +
                             SaveBrushToStr(bg.m_barBrush);
                    hasField = true;
                    }
                bgArr += wxString(hasField ? L", " : L"") + L"\"start\": " +
                         std::to_wstring(bg.m_barPositions.first);
                bgArr += L", \"end\": " + std::to_wstring(bg.m_barPositions.second);
                if (!bg.m_barDecal.empty())
                    {
                    bgArr += L", \"decal\": \"" + EscapeJsonStr(bg.m_barDecal) + L"\"";
                    }
                bgArr += L"}";
                }
            bgArr += L"]";
            node->Add(L"bar-groups", wxSimpleJSON::Create(bgArr));
            }

            // first-bar-brackets (from cached property templates)
            {
            wxString fbbArr;
            for (size_t i = 0;; ++i)
                {
                const auto idx = std::to_wstring(i);
                const auto startBlock =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].start-block");
                const auto startBlockRe =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].start-block-re");
                const auto endBlock =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].end-block");
                const auto endBlockRe =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].end-block-re");
                const auto label =
                    graph->GetPropertyTemplate(L"first-bar-brackets[" + idx + L"].label");
                if (startBlock.empty() && startBlockRe.empty())
                    {
                    break;
                    }
                if (!fbbArr.empty())
                    {
                    fbbArr += L", ";
                    }
                fbbArr += L"{";
                if (!startBlockRe.empty())
                    {
                    fbbArr += L"\"start-block-re\": \"" + EscapeJsonStr(startBlockRe) + L"\", ";
                    fbbArr += L"\"end-block-re\": \"" + EscapeJsonStr(endBlockRe) + L"\"";
                    }
                else
                    {
                    fbbArr += L"\"start-block\": \"" + EscapeJsonStr(startBlock) + L"\", ";
                    fbbArr += L"\"end-block\": \"" + EscapeJsonStr(endBlock) + L"\"";
                    }
                if (!label.empty())
                    {
                    fbbArr += L", \"label\": \"" + EscapeJsonStr(label) + L"\"";
                    }
                fbbArr += L"}";
                }
            if (!fbbArr.empty())
                {
                node->Add(L"first-bar-brackets", wxSimpleJSON::Create(L"[" + fbbArr + L"]"));
                }
            }
            // last-bar-brackets (from cached property templates)
            {
            wxString lbbArr;
            for (size_t i = 0;; ++i)
                {
                const auto idx = std::to_wstring(i);
                const auto startBlock =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].start-block");
                const auto startBlockRe =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].start-block-re");
                const auto endBlock =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].end-block");
                const auto endBlockRe =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].end-block-re");
                const auto label =
                    graph->GetPropertyTemplate(L"last-bar-brackets[" + idx + L"].label");
                if (startBlock.empty() && startBlockRe.empty())
                    {
                    break;
                    }
                if (!lbbArr.empty())
                    {
                    lbbArr += L", ";
                    }
                lbbArr += L"{";
                if (!startBlockRe.empty())
                    {
                    lbbArr += L"\"start-block-re\": \"" + EscapeJsonStr(startBlockRe) + L"\", ";
                    lbbArr += L"\"end-block-re\": \"" + EscapeJsonStr(endBlockRe) + L"\"";
                    }
                else
                    {
                    lbbArr += L"\"start-block\": \"" + EscapeJsonStr(startBlock) + L"\", ";
                    lbbArr += L"\"end-block\": \"" + EscapeJsonStr(endBlock) + L"\"";
                    }
                if (!label.empty())
                    {
                    lbbArr += L", \"label\": \"" + EscapeJsonStr(label) + L"\"";
                    }
                lbbArr += L"}";
                }
            if (!lbbArr.empty())
                {
                node->Add(L"last-bar-brackets", wxSimpleJSON::Create(L"[" + lbbArr + L"]"));
                }
            }

        // Histogram-specific options
        if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Histogram)))
            {
            const auto* histo = dynamic_cast<const Wisteria::Graphs::Histogram*>(graph);
            if (histo->GetBinningMethod() !=
                Wisteria::Graphs::Histogram::BinningMethod::BinByIntegerRange)
                {
                const auto bmStr = Wisteria::ReportEnumConvert::ConvertBinningMethodToString(
                    histo->GetBinningMethod());
                if (bmStr.has_value())
                    {
                    node->Add(L"binning-method", bmStr.value());
                    }
                }
            if (histo->GetIntervalDisplay() !=
                Wisteria::Graphs::Histogram::IntervalDisplay::Cutpoints)
                {
                const auto idStr = Wisteria::ReportEnumConvert::ConvertIntervalDisplayToString(
                    histo->GetIntervalDisplay());
                if (idStr.has_value())
                    {
                    node->Add(L"interval-display", idStr.value());
                    }
                }
            if (histo->GetRoundingMethod() != Wisteria::RoundingMethod::NoRounding)
                {
                const auto rmStr = Wisteria::ReportEnumConvert::ConvertRoundingMethodToString(
                    histo->GetRoundingMethod());
                if (rmStr.has_value())
                    {
                    node->Add(L"rounding", rmStr.value());
                    }
                }
            if (!histo->IsShowingFullRangeOfValues())
                {
                node->Add(L"show-full-range", false);
                }
            if (histo->GetBinsStart().has_value())
                {
                node->Add(L"bins-start", histo->GetBinsStart().value());
                }
            if (histo->GetSuggestedBinCount().has_value())
                {
                node->Add(L"suggested-bin-count",
                          static_cast<double>(histo->GetSuggestedBinCount().value()));
                }
            if (histo->GetMaxNumberOfBins() != 255)
                {
                node->Add(L"max-bin-count", static_cast<double>(histo->GetMaxNumberOfBins()));
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::PieChart)))
        {
        const auto* pieChart = dynamic_cast<const Wisteria::Graphs::PieChart*>(graph);
        if (pieChart->GetPieSliceEffect() != Wisteria::PieSliceEffect::Solid)
            {
            const auto seStr = Wisteria::ReportEnumConvert::ConvertPieSliceEffectToString(
                pieChart->GetPieSliceEffect());
            if (seStr.has_value())
                {
                node->Add(L"pie-slice-effect", seStr.value());
                }
            }

        // serialize image paths when the slice effect uses images;
        // blank entries are preserved so the slice falls back to the brush on reload
        if (pieChart->GetPieSliceEffect() == Wisteria::PieSliceEffect::Image)
            {
            const auto imgPaths = pieChart->GetPropertyTemplate(L"image-paths");
            if (!imgPaths.empty())
                {
                wxArrayString paths;
                wxStringTokenizer tokenizer(imgPaths, L"\t", wxTOKEN_RET_EMPTY_ALL);
                while (tokenizer.HasMoreTokens())
                    {
                    paths.Add(tokenizer.GetNextToken());
                    }

                wxString arr = L"[";
                for (size_t i = 0; i < paths.GetCount(); ++i)
                    {
                    if (i > 0)
                        {
                        arr += L", ";
                        }
                    if (paths[i].empty())
                        {
                        // null image — encoded as an empty path so ReportBuilder
                        // produces an unusable bundle and the brush is used
                        arr += L"{\"path\": \"\"}";
                        }
                    else
                        {
                        arr +=
                            L"{\"path\": \"" + EscapeJsonStr(MakeRelativePath(paths[i])) + L"\"}";
                        }
                    }
                arr += L"]";
                node->Add(L"image-scheme", wxSimpleJSON::Create(arr));
                }
            }
        if (pieChart->GetPieStyle() != Wisteria::PieStyle::None)
            {
            const auto psStr =
                Wisteria::ReportEnumConvert::ConvertPieStyleToString(pieChart->GetPieStyle());
            if (psStr.has_value())
                {
                node->Add(L"pie-style", psStr.value());
                }
            }
        if (pieChart->GetLabelPlacement() != Wisteria::LabelPlacement::Flush)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                pieChart->GetLabelPlacement());
            if (lpStr.has_value())
                {
                node->Add(L"label-placement", lpStr.value());
                }
            }
        if (pieChart->GetOuterPieMidPointLabelDisplay() != Wisteria::BinLabelDisplay::BinPercentage)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                pieChart->GetOuterPieMidPointLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"outer-pie-midpoint-label-display", blStr.value());
                }
            }
        if (pieChart->GetInnerPieMidPointLabelDisplay() != Wisteria::BinLabelDisplay::BinPercentage)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                pieChart->GetInnerPieMidPointLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"inner-pie-midpoint-label-display", blStr.value());
                }
            }
        if (pieChart->GetOuterLabelDisplay() != Wisteria::BinLabelDisplay::BinName)
            {
            const auto blStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                pieChart->GetOuterLabelDisplay());
            if (blStr.has_value())
                {
                node->Add(L"outer-label-display", blStr.value());
                }
            }
        if (!pieChart->IsShowingOuterPieLabels())
            {
            node->Add(L"include-outer-pie-labels", false);
            }
        if (!pieChart->IsShowingInnerPieLabels())
            {
            node->Add(L"include-inner-pie-labels", false);
            }
        if (pieChart->IsUsingColorLabels())
            {
            node->Add(L"color-labels", true);
            }
        if (pieChart->HasDynamicMargins())
            {
            node->Add(L"dynamic-margins", true);
            }
        if (pieChart->GetGhostOpacity() != Wisteria::Settings::GHOST_OPACITY)
            {
            node->Add(L"ghost-opacity", static_cast<double>(pieChart->GetGhostOpacity()));
            }

        // donut hole
        auto donutHoleNode = wxSimpleJSON::Create(wxSimpleJSON::JSONType::IS_OBJECT);
        donutHoleNode->Add(L"include", pieChart->IsIncludingDonutHole());
        donutHoleNode->Add(L"proportion", pieChart->GetDonutHoleProportion());
        donutHoleNode->Add(L"color", ColorToStr(pieChart->GetDonutHoleColor()));
        donutHoleNode->Add(L"label", wxSimpleJSON::Create(
                                         SaveLabelPropertiesToStr(pieChart->GetDonutHoleLabel())));
        node->Add(L"donut-hole", donutHoleNode);

        // showcase-slices
        const auto showcaseMode = pieChart->GetShowcaseMode();
        if (showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::ExplicitList)
            {
            // array form (indexed templates)
            wxString showcaseSliceArr;
            for (size_t i = 0;; ++i)
                {
                const auto val =
                    graph->GetPropertyTemplate(L"showcase-slices[" + std::to_wstring(i) + L"]");
                if (val.empty())
                    {
                    break;
                    }
                if (!showcaseSliceArr.empty())
                    {
                    showcaseSliceArr += L", ";
                    }
                showcaseSliceArr += L"\"" + EscapeJsonStr(val) + L"\"";
                }
            if (!showcaseSliceArr.empty())
                {
                node->Add(L"showcase-slices", wxSimpleJSON::Create(L"[" + showcaseSliceArr + L"]"));
                }
            }
        else if (showcaseMode != Wisteria::Graphs::PieChart::ShowcaseMode::None)
            {
            // object form
            wxString scObj = L"{";
            const bool isInner =
                (showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::LargestInner ||
                 showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::SmallestInner);
            scObj += isInner ? L"\"pie\": \"inner\"" : L"\"pie\": \"outer\"";
            const bool isLargest =
                (showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::LargestOuter ||
                 showcaseMode == Wisteria::Graphs::PieChart::ShowcaseMode::LargestInner);
            scObj += isLargest ? L", \"category\": \"largest\"" : L", \"category\": \"smallest\"";
            if (isInner)
                {
                scObj += pieChart->IsShowcaseByGroup() ? L", \"by-group\": true" :
                                                         L", \"by-group\": false";
                scObj += pieChart->IsShowcaseShowingOuterPieMidPointLabels() ?
                             L", \"show-outer-pie-midpoint-labels\": true" :
                             L", \"show-outer-pie-midpoint-labels\": false";
                }
            scObj += L"}";
            node->Add(L"showcase-slices", wxSimpleJSON::Create(scObj));
            }
        // showcased-ring-labels (for any showcase mode)
        if (showcaseMode != Wisteria::Graphs::PieChart::ShowcaseMode::None)
            {
            const auto periStr = Wisteria::ReportEnumConvert::ConvertPerimeterToString(
                pieChart->GetShowcasedRingLabels());
            if (periStr.has_value() &&
                pieChart->GetShowcasedRingLabels() != Wisteria::Perimeter::Outer)
                {
                node->Add(L"showcased-ring-labels", periStr.value());
                }
            }
        // inner-pie-line-pen
        const auto& ipPen = pieChart->GetInnerPieConnectionLinePen();
        if (ipPen.IsOk() && ipPen != wxNullPen)
            {
            node->Add(L"inner-pie-line-pen", wxSimpleJSON::Create(SavePenToStr(ipPen)));
            }
        // margin notes
        if (!pieChart->GetLeftMarginNote().GetText().empty())
            {
            const auto marginNode = SaveLabel(&pieChart->GetLeftMarginNote(), canvas);
            if (marginNode)
                {
                node->Add(L"left-margin-note", marginNode);
                }
            }
        if (!pieChart->GetRightMarginNote().GetText().empty())
            {
            const auto marginNode = SaveLabel(&pieChart->GetRightMarginNote(), canvas);
            if (marginNode)
                {
                node->Add(L"right-margin-note", marginNode);
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::BoxPlot)))
        {
        const auto* boxPlot = dynamic_cast<const Wisteria::Graphs::BoxPlot*>(graph);
        const auto beStr =
            Wisteria::ReportEnumConvert::ConvertBoxEffectToString(boxPlot->GetBoxEffect());
        if (beStr.has_value() && boxPlot->GetBoxEffect() != Wisteria::BoxEffect::Solid)
            {
            node->Add(L"box-effect", beStr.value());
            }
        if (boxPlot->IsShowingAllPoints())
            {
            node->Add(L"show-all-points", true);
            }
        if (boxPlot->IsShowingLabels())
            {
            node->Add(L"show-labels", true);
            }
        if (!boxPlot->IsShowingMidpointConnection())
            {
            node->Add(L"show-midpoint-connection", false);
            }

        // serialize image paths for image-based box effects
        const auto imgPaths = boxPlot->GetPropertyTemplate(L"image-paths");
        const auto imgEffect = boxPlot->GetPropertyTemplate(L"image-effect");
        if (!imgPaths.empty())
            {
            // helper to build an image object JSON string with optional effect
            const auto buildImageObj = [&](const wxString& path) -> wxString
            {
                wxString obj = L"{\"path\": \"" + EscapeJsonStr(MakeRelativePath(path)) + L"\"";
                if (!imgEffect.empty())
                    {
                    obj += L", \"effect\": \"" + EscapeJsonStr(imgEffect) + L"\"";
                    }
                obj += L"}";
                return obj;
            };

            wxArrayString paths;
            wxStringTokenizer tokenizer(imgPaths, L"\t");
            while (tokenizer.HasMoreTokens())
                {
                paths.Add(tokenizer.GetNextToken());
                }

            const auto boxEff = boxPlot->GetBoxEffect();
            if (boxEff == Wisteria::BoxEffect::StippleImage && !paths.empty())
                {
                node->Add(L"stipple-image", wxSimpleJSON::Create(buildImageObj(paths[0])));
                }
            else if (boxEff == Wisteria::BoxEffect::CommonImage ||
                     boxEff == Wisteria::BoxEffect::Image)
                {
                wxString arr = L"[";
                for (size_t i = 0; i < paths.GetCount(); ++i)
                    {
                    if (i > 0)
                        {
                        arr += L", ";
                        }
                    arr += buildImageObj(paths[i]);
                    }
                arr += L"]";
                node->Add(L"image-scheme", wxSimpleJSON::Create(arr));
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ChernoffFacesPlot)))
        {
        const auto* chernoff = dynamic_cast<const Wisteria::Graphs::ChernoffFacesPlot*>(graph);
        node->Add(L"face-color", ColorToStr(chernoff->GetFaceColor()));
        node->Add(L"show-labels", chernoff->IsShowingLabels());
        node->Add(L"enhanced-legend", chernoff->GetPropertyTemplate(L"enhanced-legend") == L"true");
        node->Add(L"outline-color", ColorToStr(chernoff->GetOutlineColor()));
        const auto gStr = Wisteria::ReportEnumConvert::ConvertGenderToString(chernoff->GetGender());
        if (gStr.has_value())
            {
            node->Add(L"gender", gStr.value());
            }
        node->Add(L"eye-color", ColorToStr(chernoff->GetEyeColor()));
        node->Add(L"hair-color", ColorToStr(chernoff->GetHairColor()));
        node->Add(L"lipstick-color", ColorToStr(chernoff->GetLipstickColor()));
        const auto hsStr =
            Wisteria::ReportEnumConvert::ConvertHairStyleToString(chernoff->GetHairStyle());
        if (hsStr.has_value())
            {
            node->Add(L"hair-style", hsStr.value());
            }
        const auto fhStr =
            Wisteria::ReportEnumConvert::ConvertFacialHairToString(chernoff->GetFacialHair());
        if (fhStr.has_value())
            {
            node->Add(L"facial-hair", fhStr.value());
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WaffleChart)))
        {
        const auto* waffle = dynamic_cast<const Wisteria::Graphs::WaffleChart*>(graph);
        // shapes
        const auto& shapes = waffle->GetShapes();
        if (!shapes.empty())
            {
            wxString shapesArr = L"[";
            for (size_t i = 0; i < shapes.size(); ++i)
                {
                if (i > 0)
                    {
                    shapesArr += L", ";
                    }
                const auto& shp = shapes[i];
                shapesArr += L"{";
                const auto iconStr =
                    Wisteria::ReportEnumConvert::ConvertIconToString(shp.GetShape());
                shapesArr += L"\"icon\": \"" +
                             (iconStr.has_value() ? iconStr.value() : wxString(L"square")) + L"\"";
                const auto repeatTmpl = shp.GetPropertyTemplate(L"repeat");
                if (!repeatTmpl.empty())
                    {
                    shapesArr += L", \"repeat\": \"" + EscapeJsonStr(repeatTmpl) + L"\"";
                    }
                else if (shp.GetRepeatCount() != 1)
                    {
                    shapesArr += L", \"repeat\": " + std::to_wstring(shp.GetRepeatCount());
                    }
                const auto& pen = shp.GetPen();
                if (pen.IsOk() && pen != wxNullPen &&
                    !(pen.GetColour() == *wxBLACK && pen.GetColour().IsOpaque() &&
                      pen.GetWidth() <= 1 && pen.GetStyle() == wxPENSTYLE_SOLID))
                    {
                    shapesArr += L", \"pen\": " + SavePenToStr(pen);
                    }
                const auto& brush = shp.GetBrush();
                if (brush.IsOk() && brush != wxNullBrush &&
                    !(brush.GetColour() == *wxWHITE && brush.GetStyle() == wxBRUSHSTYLE_SOLID))
                    {
                    shapesArr += L", \"brush\": " + SaveBrushToStr(brush);
                    }
                if (!shp.GetText().empty())
                    {
                    shapesArr +=
                        L", \"label\": {\"text\": \"" + EscapeJsonStr(shp.GetText()) + L"\"}";
                    }
                const auto fillPctTmpl = shp.GetPropertyTemplate(L"fill-percent");
                if (!fillPctTmpl.empty())
                    {
                    shapesArr += L", \"fill-percent\": \"" + EscapeJsonStr(fillPctTmpl) + L"\"";
                    }
                else if (shp.GetFillPercent() < math_constants::full)
                    {
                    shapesArr += wxString::Format(L", \"fill-percent\": %g", shp.GetFillPercent());
                    }
                shapesArr += L"}";
                }
            shapesArr += L"]";
            node->Add(L"shapes", wxSimpleJSON::Create(shapesArr));
            }
        // grid-round
        if (waffle->GetGridRounding().has_value())
            {
            const auto& gr = waffle->GetGridRounding().value();
            node->Add(L"grid-round", wxSimpleJSON::Create(wxString::Format(
                                         L"{\"cell-count\": %zu, \"shape-index\": %zu}",
                                         gr.m_numberOfCells, gr.m_shapesIndex)));
            }
        // row-count
        if (waffle->GetRowCount().has_value())
            {
            node->Add(L"row-count", static_cast<double>(waffle->GetRowCount().value()));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::CandlestickPlot)))
        {
        const auto* candlePlot = dynamic_cast<const Wisteria::Graphs::CandlestickPlot*>(graph);
        if (candlePlot->GetPlotType() != Wisteria::Graphs::CandlestickPlot::PlotType::Candlestick)
            {
            const auto ptStr = Wisteria::ReportEnumConvert::ConvertCandlestickPlotTypeToString(
                candlePlot->GetPlotType());
            if (ptStr.has_value())
                {
                node->Add(L"plot-type", ptStr.value());
                }
            }
        const auto& gainBrush = candlePlot->GetGainBrush();
        if (gainBrush.IsOk() && gainBrush != wxNullBrush &&
            gainBrush.GetColour() !=
                Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Green))
            {
            node->Add(L"gain-brush", wxSimpleJSON::Create(SaveBrushToStr(gainBrush)));
            }
        const auto& lossBrush = candlePlot->GetLossBrush();
        if (lossBrush.IsOk() && lossBrush != wxNullBrush &&
            lossBrush.GetColour() !=
                Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Red))
            {
            node->Add(L"loss-brush", wxSimpleJSON::Create(SaveBrushToStr(lossBrush)));
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::GanttChart)))
        {
        const auto* gantt = dynamic_cast<const Wisteria::Graphs::GanttChart*>(graph);
        if (gantt->GetDateDisplayInterval() != Wisteria::DateInterval::FiscalQuarterly)
            {
            const auto diStr = Wisteria::ReportEnumConvert::ConvertDateIntervalToString(
                gantt->GetDateDisplayInterval());
            if (diStr.has_value())
                {
                node->Add(L"date-interval", diStr.value());
                }
            }
        if (gantt->GetFiscalYearType() != Wisteria::FiscalYear::USBusiness)
            {
            const auto fyStr =
                Wisteria::ReportEnumConvert::ConvertFiscalYearToString(gantt->GetFiscalYearType());
            if (fyStr.has_value())
                {
                node->Add(L"fy-type", fyStr.value());
                }
            }
        if (gantt->GetLabelDisplay() != Wisteria::Graphs::GanttChart::TaskLabelDisplay::Days)
            {
            const auto tlStr = Wisteria::ReportEnumConvert::ConvertTaskLabelDisplayToString(
                gantt->GetLabelDisplay());
            if (tlStr.has_value())
                {
                node->Add(L"task-label-display", tlStr.value());
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::SankeyDiagram)))
        {
        const auto* sankey = dynamic_cast<const Wisteria::Graphs::SankeyDiagram*>(graph);
        if (sankey->GetGroupLabelDisplay() != Wisteria::BinLabelDisplay::BinName)
            {
            const auto glStr = Wisteria::ReportEnumConvert::ConvertBinLabelDisplayToString(
                sankey->GetGroupLabelDisplay());
            if (glStr.has_value())
                {
                node->Add(L"group-label-display", glStr.value());
                }
            }
        if (sankey->GetColumnHeaderDisplay() != Wisteria::GraphColumnHeader::NoDisplay)
            {
            const auto ghStr = Wisteria::ReportEnumConvert::ConvertGraphColumnHeaderToString(
                sankey->GetColumnHeaderDisplay());
            if (ghStr.has_value())
                {
                node->Add(L"group-header-display", ghStr.value());
                }
            }
        // column-headers (cached as indexed templates)
        wxString colArr;
        for (size_t i = 0;; ++i)
            {
            const auto val =
                sankey->GetPropertyTemplate(L"column-headers[" + std::to_wstring(i) + L"]");
            if (val.empty())
                {
                break;
                }
            if (!colArr.empty())
                {
                colArr += L", ";
                }
            colArr += L"\"" + EscapeJsonStr(val) + L"\"";
            }
        if (!colArr.empty())
            {
            node->Add(L"column-headers", wxSimpleJSON::Create(L"[" + colArr + L"]"));
            }
        if (sankey->GetFlowShape() != Wisteria::FlowShape::Curvy)
            {
            const auto fsStr =
                Wisteria::ReportEnumConvert::ConvertFlowShapeToString(sankey->GetFlowShape());
            if (fsStr.has_value())
                {
                node->Add(L"flow-shape", fsStr.value());
                }
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::HeatMap)))
        {
        const auto* heatmap = dynamic_cast<const Wisteria::Graphs::HeatMap*>(graph);
        if (heatmap->GetGroupColumnCount().has_value())
            {
            node->Add(L"group-column-count",
                      static_cast<double>(heatmap->GetGroupColumnCount().value()));
            }
        if (!heatmap->IsShowingGroupHeaders())
            {
            node->Add(L"show-group-header", false);
            }
        if (!heatmap->GetGroupHeaderPrefix().empty())
            {
            node->Add(L"group-header-prefix", heatmap->GetGroupHeaderPrefix());
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::WinLossSparkline)))
        {
        const auto* sparkline = dynamic_cast<const Wisteria::Graphs::WinLossSparkline*>(graph);
        if (!sparkline->IsHighlightingBestRecords())
            {
            node->Add(L"highlight-best-records", false);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::LRRoadmap)))
        {
        const auto* lrRoadmap = dynamic_cast<const Wisteria::Graphs::LRRoadmap*>(graph);
        // p-value-threshold
        if (lrRoadmap->GetPValueThreshold().has_value())
            {
            node->Add(L"p-value-threshold", lrRoadmap->GetPValueThreshold().value());
            }
        // predictors-to-include
        if (lrRoadmap->GetPredictorsToInclude().has_value())
            {
            const auto preds = static_cast<int>(lrRoadmap->GetPredictorsToInclude().value());
            wxString predsArr = L"[";
            bool hasEntry{ false };
            if (preds ==
                (Wisteria::Influence::InfluencePositive | Wisteria::Influence::InfluenceNegative |
                 Wisteria::Influence::InfluenceNeutral))
                {
                predsArr += L"\"all\"";
                hasEntry = true;
                }
            else
                {
                if (preds & Wisteria::Influence::InfluencePositive)
                    {
                    if (hasEntry)
                        {
                        predsArr += L", ";
                        }
                    predsArr += L"\"positive\"";
                    hasEntry = true;
                    }
                if (preds & Wisteria::Influence::InfluenceNegative)
                    {
                    if (hasEntry)
                        {
                        predsArr += L", ";
                        }
                    predsArr += L"\"negative\"";
                    hasEntry = true;
                    }
                if (preds & Wisteria::Influence::InfluenceNeutral)
                    {
                    if (hasEntry)
                        {
                        predsArr += L", ";
                        }
                    predsArr += L"\"neutral\"";
                    hasEntry = true;
                    }
                }
            predsArr += L"]";
            if (hasEntry)
                {
                node->Add(L"predictors-to-include", wxSimpleJSON::Create(predsArr));
                }
            }
        // shared roadmap properties
        const auto* roadmap = dynamic_cast<const Wisteria::Graphs::Roadmap*>(graph);
        const auto& roadPen = roadmap->GetRoadPen();
        if (roadPen.IsOk() && roadPen != wxNullPen &&
            !(roadPen.GetColour() == *wxBLACK && roadPen.GetColour().IsOpaque() &&
              roadPen.GetWidth() == 10 && roadPen.GetStyle() == wxPENSTYLE_SOLID))
            {
            node->Add(L"road-pen", wxSimpleJSON::Create(SavePenToStr(roadPen)));
            }
        const auto& lanePen = roadmap->GetLaneSeparatorPen();
        if (lanePen.IsOk() && lanePen != wxNullPen)
            {
            node->Add(L"lane-separator-pen", wxSimpleJSON::Create(SavePenToStr(lanePen)));
            }
        if (roadmap->GetLabelPlacement() != Wisteria::LabelPlacement::Flush)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                roadmap->GetLabelPlacement());
            if (lpStr.has_value())
                {
                node->Add(L"label-placement", lpStr.value());
                }
            }
        if (roadmap->GetLaneSeparatorStyle() !=
            Wisteria::Graphs::Roadmap::LaneSeparatorStyle::SingleLine)
            {
            const auto lsStr = Wisteria::ReportEnumConvert::ConvertLaneSeparatorStyleToString(
                roadmap->GetLaneSeparatorStyle());
            if (lsStr.has_value())
                {
                node->Add(L"lane-separator-style", lsStr.value());
                }
            }
        if (roadmap->GetRoadStopTheme() !=
            Wisteria::Graphs::Roadmap::RoadStopTheme::LocationMarkers)
            {
            const auto rsStr = Wisteria::ReportEnumConvert::ConvertRoadStopThemeToString(
                roadmap->GetRoadStopTheme());
            if (rsStr.has_value())
                {
                node->Add(L"road-stop-theme", rsStr.value());
                }
            }
        if (roadmap->GetMarkerLabelDisplay() !=
            Wisteria::Graphs::Roadmap::MarkerLabelDisplay::NameAndValue)
            {
            const auto mlStr = Wisteria::ReportEnumConvert::ConvertMarkerLabelDisplayToString(
                roadmap->GetMarkerLabelDisplay());
            if (mlStr.has_value())
                {
                node->Add(L"marker-label-display", mlStr.value());
                }
            }
        if (lrRoadmap->HasDefaultCaption())
            {
            node->Add(L"default-caption", true);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::ProConRoadmap)))
        {
        const auto* pcRoadmap = dynamic_cast<const Wisteria::Graphs::ProConRoadmap*>(graph);
        // minimum-count
        if (pcRoadmap->GetMinimumCount().has_value())
            {
            node->Add(L"minimum-count", static_cast<double>(pcRoadmap->GetMinimumCount().value()));
            }
        // positive/negative legend labels
        node->Add(L"positive-legend-label", pcRoadmap->GetPositiveLabel());
        node->Add(L"negative-legend-label", pcRoadmap->GetNegativeLabel());
        // shared roadmap properties
        const auto* roadmap = dynamic_cast<const Wisteria::Graphs::Roadmap*>(graph);
        const auto& roadPen = roadmap->GetRoadPen();
        if (roadPen.IsOk() && roadPen != wxNullPen &&
            !(roadPen.GetColour() == *wxBLACK && roadPen.GetColour().IsOpaque() &&
              roadPen.GetWidth() == 10 && roadPen.GetStyle() == wxPENSTYLE_SOLID))
            {
            node->Add(L"road-pen", wxSimpleJSON::Create(SavePenToStr(roadPen)));
            }
        const auto& lanePen = roadmap->GetLaneSeparatorPen();
        if (lanePen.IsOk() && lanePen != wxNullPen)
            {
            node->Add(L"lane-separator-pen", wxSimpleJSON::Create(SavePenToStr(lanePen)));
            }
        if (roadmap->GetLabelPlacement() != Wisteria::LabelPlacement::Flush)
            {
            const auto lpStr = Wisteria::ReportEnumConvert::ConvertLabelPlacementToString(
                roadmap->GetLabelPlacement());
            if (lpStr.has_value())
                {
                node->Add(L"label-placement", lpStr.value());
                }
            }
        if (roadmap->GetLaneSeparatorStyle() !=
            Wisteria::Graphs::Roadmap::LaneSeparatorStyle::SingleLine)
            {
            const auto lsStr = Wisteria::ReportEnumConvert::ConvertLaneSeparatorStyleToString(
                roadmap->GetLaneSeparatorStyle());
            if (lsStr.has_value())
                {
                node->Add(L"lane-separator-style", lsStr.value());
                }
            }
        if (roadmap->GetRoadStopTheme() !=
            Wisteria::Graphs::Roadmap::RoadStopTheme::LocationMarkers)
            {
            const auto rsStr = Wisteria::ReportEnumConvert::ConvertRoadStopThemeToString(
                roadmap->GetRoadStopTheme());
            if (rsStr.has_value())
                {
                node->Add(L"road-stop-theme", rsStr.value());
                }
            }
        if (roadmap->GetMarkerLabelDisplay() !=
            Wisteria::Graphs::Roadmap::MarkerLabelDisplay::NameAndAbsoluteValue)
            {
            const auto mlStr = Wisteria::ReportEnumConvert::ConvertMarkerLabelDisplayToString(
                roadmap->GetMarkerLabelDisplay());
            if (mlStr.has_value())
                {
                node->Add(L"marker-label-display", mlStr.value());
                }
            }
        if (pcRoadmap->HasDefaultCaption())
            {
            node->Add(L"default-caption", true);
            }
        }
    else if (graph->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Table)))
        {
        const auto* table = dynamic_cast<const Wisteria::Graphs::Table*>(graph);
        // default-borders (only if any are false)
        if (!table->IsShowingTopBorder() || !table->IsShowingRightBorder() ||
            !table->IsShowingBottomBorder() || !table->IsShowingLeftBorder())
            {
            node->Add(L"default-borders",
                      wxSimpleJSON::Create(wxString::Format(
                          L"[%s, %s, %s, %s]", table->IsShowingTopBorder() ? L"true" : L"false",
                          table->IsShowingRightBorder() ? L"true" : L"false",
                          table->IsShowingBottomBorder() ? L"true" : L"false",
                          table->IsShowingLeftBorder() ? L"true" : L"false")));
            }
        // min-width-proportion
        if (table->GetMinWidthProportion().has_value())
            {
            node->Add(L"min-width-proportion", table->GetMinWidthProportion().value());
            }
        // min-height-proportion
        if (table->GetMinHeightProportion().has_value())
            {
            node->Add(L"min-height-proportion", table->GetMinHeightProportion().value());
            }
        // clear-trailing-row-formatting
        if (table->IsClearingTrailingRowFormatting())
            {
            node->Add(L"clear-trailing-row-formatting", true);
            }
        // highlight-pen (default is solid red, 1px)
        const auto& hlPen = table->GetHighlightPen();
        if (hlPen.IsOk() && hlPen != wxNullPen &&
            !(hlPen.GetColour() ==
                  Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::Red) &&
              hlPen.GetWidth() == 1 && hlPen.GetStyle() == wxPENSTYLE_SOLID))
            {
            node->Add(L"highlight-pen", wxSimpleJSON::Create(SavePenToStr(hlPen)));
            }
        // cached procedural properties
        for (const auto& prop : { L"variables",
                                  L"row-sort",
                                  L"insert-group-header",
                                  L"row-group",
                                  L"column-group",
                                  L"alternate-row-color",
                                  L"row-add",
                                  L"row-suppression",
                                  L"column-suppression",
                                  L"row-formatting",
                                  L"row-color",
                                  L"row-bold",
                                  L"row-borders",
                                  L"row-content-align",
                                  L"column-formatting",
                                  L"column-color",
                                  L"column-bold",
                                  L"column-borders",
                                  L"column-content-align",
                                  L"column-highlight",
                                  L"aggregates",
                                  L"row-totals",
                                  L"cell-update",
                                  L"cell-annotations",
                                  L"footnotes",
                                  L"ui.bold-header-row",
                                  L"ui.center-header-row",
                                  L"ui.bold-first-column" })
            {
            const auto cachedJson = graph->GetPropertyTemplate(prop);
            if (!cachedJson.empty())
                {
                node->Add(wxString{ prop }, wxSimpleJSON::Create(cachedJson));
                }
            }
        // transpose (cached as "true" string)
        if (graph->GetPropertyTemplate(L"transpose") == L"true")
            {
            node->Add(L"transpose", true);
            }
        // link-id (cached as numeric string)
        const auto linkIdStr = graph->GetPropertyTemplate(L"link-id");
        if (!linkIdStr.empty())
            {
            long linkIdVal{ 0 };
            if (wxString(linkIdStr).ToLong(&linkIdVal))
                {
                node->Add(L"link-id", static_cast<double>(linkIdVal));
                }
            }
        }

    return node;
    }

//-------------------------------------------
wxSimpleJSON::Ptr_t WisteriaDoc::SavePageItem(const Wisteria::GraphItems::GraphItemBase* item,
                                              const Wisteria::Canvas* canvas) const
    {
    if (item == nullptr)
        {
        return wxSimpleJSON::Ptr_t{};
        }

    // Graph2D before other types (it inherits from GraphItemBase directly)
    if (item->IsKindOf(wxCLASSINFO(Wisteria::Graphs::Graph2D)))
        {
        return SaveGraphByType(dynamic_cast<const Wisteria::Graphs::Graph2D*>(item), canvas);
        }

    // (FillableShape before Shape since it derives from Shape)
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::FillableShape)))
        {
        return SaveFillableShape(dynamic_cast<const Wisteria::GraphItems::FillableShape*>(item),
                                 canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Shape)))
        {
        return SaveShape(dynamic_cast<const Wisteria::GraphItems::Shape*>(item), canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Image)))
        {
        return SaveImage(dynamic_cast<const Wisteria::GraphItems::Image*>(item), canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Axis)))
        {
        return SaveCommonAxis(dynamic_cast<const Wisteria::GraphItems::Axis*>(item), canvas);
        }
    if (item->IsKindOf(wxCLASSINFO(Wisteria::GraphItems::Label)))
        {
        return SaveLabel(dynamic_cast<const Wisteria::GraphItems::Label*>(item), canvas);
        }

    return wxSimpleJSON::Ptr_t{};
    }

//-------------------------------------------
wxString WisteriaDoc::MakeRelativePath(const wxString& filePath) const
    {
    if (!GetFilename().empty())
        {
        wxFileName fn{ filePath };
        if (fn.IsAbsolute())
            {
            const wxFileName projectDir{ GetFilename() };
            fn.MakeRelativeTo(projectDir.GetPath());
            return fn.GetFullPath(wxPATH_UNIX);
            }
        }
    return filePath;
    }

//-------------------------------------------
wxString WisteriaDoc::ResolveFilePath(const wxString& filePath) const
    {
    if (!GetFilename().empty())
        {
        wxFileName fn{ filePath };
        if (fn.IsRelative())
            {
            fn.MakeAbsolute(wxFileName{ GetFilename() }.GetPath());
            return fn.GetFullPath();
            }
        }
    return filePath;
    }
