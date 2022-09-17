#include "reportbuilder.h"

using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria
    {
    //---------------------------------------------------
    std::vector<Canvas*> ReportBuilder::LoadConfigurationFile(const wxString& filePath,
                                                              wxWindow* parent)
        {
        // reset from previous calls
        m_commonAxesPlaceholders.clear();
        m_name.clear();
        m_datasets.clear();
        m_dpiScaleFactor = parent->GetDPIScaleFactor();

        m_configFilePath = filePath;

        std::vector<Canvas*> reportPages;
        std::vector<std::shared_ptr<Wisteria::Graphs::Graph2D>> embeddedGraphs;

        wxASSERT_MSG(parent, L"Parent window must not be null when building a canvas!");
        if (parent == nullptr)
            { return reportPages; }
        const auto json = wxSimpleJSON::LoadFile(m_configFilePath);
        if (!json->IsOk())
            {
            wxMessageBox(json->GetLastError(),
                         _(L"Configuration File Parsing Error"), wxOK|wxICON_WARNING|wxCENTRE);
            return reportPages;
            }

        const auto reportNameNode = json->GetProperty(L"name");
        if (reportNameNode->IsOk())
            { m_name = reportNameNode->GetValueString(); }

        // print settings
        wxPrintData reportPrintSettings;
        const auto printNode = json->GetProperty(L"print");
        if (printNode->IsOk())
            {
            const auto orientation = printNode->GetProperty(L"orientation")->GetValueString();
            if (orientation.CmpNoCase(L"horizontal") == 0 ||
                orientation.CmpNoCase(L"landscape") == 0)
                { reportPrintSettings.SetOrientation(wxPrintOrientation::wxLANDSCAPE); }
            else if (orientation.CmpNoCase(L"vertical") == 0 ||
                orientation.CmpNoCase(L"portrait") == 0)
                { reportPrintSettings.SetOrientation(wxPrintOrientation::wxPORTRAIT); }
            }

        const auto datasetsNode = json->GetProperty(L"datasets");
        try
            { LoadDatasets(datasetsNode); }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Datasets Section Error"), wxOK|wxICON_WARNING|wxCENTRE);
            return reportPages;
            }

        try
            { LoadConstants(json->GetProperty(L"constants")); }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Constants Section Error"), wxOK|wxICON_WARNING|wxCENTRE);
            return reportPages;
            }

        // start loading the pages
        const auto pagesProperty = json->GetProperty(L"pages");
        if (pagesProperty->IsOk())
            {
            const auto pages = pagesProperty->GetValueArrayObject();
            for (const auto& page : pages)
                {
                if (page->IsOk())
                    {
                    // create the canvas used for the page
                    auto canvas = new Canvas(parent);
                    canvas->SetLabel(page->GetProperty(L"name")->GetValueString());

                    // copy print settings from report
                    canvas->GetPrinterSettings().SetOrientation(reportPrintSettings.GetOrientation());

                    size_t rowCount{ 0 };
                    const auto rowsProperty = page->GetProperty(L"rows");
                    if (rowsProperty->IsOk())
                        {
                        size_t currentRow{ 0 }, currentColumn{ 0 };
                        const auto rows = rowsProperty->GetValueArrayObject();
                        rowCount = rows.size();
                        // Empty page? Go to next one.
                        if (rows.size() == 0)
                            { continue; }
                        canvas->SetFixedObjectsGridSize(rows.size(), 1);
                        for (const auto& row : rows)
                            {
                            const auto itemsProperty = row->GetProperty(L"items");
                            if (itemsProperty->IsOk())
                                {
                                currentColumn = 0;
                                auto items = itemsProperty->GetValueArrayObject();
                                for (const auto& item : items)
                                    {
                                    const auto typeProperty = item->GetProperty(L"type");
                                    // load the item into the grid cell(s)
                                    if (typeProperty->IsOk())
                                        {
                                        try
                                            {
                                            /* Along with adding graphs to the canvas, we also keep a list
                                               of these graphs in case we need to connect any of them to
                                               a common axis.

                                               Graph loading functions will load the graph to the canvas
                                               themselves because they may need to add an accompanying legend,
                                               which that function will add to the canvas also.

                                               Other objects like labels and images will be added to the canvas
                                               here though, as we know it will just be that one object.*/
                                            if (typeProperty->GetValueString().CmpNoCase(L"line-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(
                                                    LoadLinePlot(item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"pie-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(
                                                    LoadPieChart(item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"histogram") == 0)
                                                {
                                                embeddedGraphs.push_back(
                                                    LoadHistogram(item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"categorical-bar-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(
                                                    LoadCategoricalBarChart(item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"label") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    LoadLabel(item, GraphItems::Label()));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"image") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    LoadImage(item));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"table") == 0)
                                                {
                                                LoadTable(item, canvas, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"common-axis") == 0)
                                                {
                                                // Common axis cannot be created until we know all its children
                                                // have been created. Add a placeholder for now and circle back
                                                // after all other items have been added to the grid.
                                                canvas->SetFixedObject(currentRow, currentColumn, nullptr);
                                                LoadCommonAxis(item, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"shape") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    LoadShape(item));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"fillable-shape") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    LoadFillableShape(item));
                                                }
                                            // explicitly null item is a placeholder,
                                            // or possibly a blank row that will be consumed by the
                                            // previous row to make it twice as tall as others
                                            else if (typeProperty->IsNull())
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    nullptr);
                                                }
                                            }
                                        // show error, but OK to keep going
                                        catch (const std::exception& err)
                                            {
                                            wxMessageBox(wxString::FromUTF8(
                                                         wxString::FromUTF8(err.what())),
                                                         _(L"Canvas Item Error"), wxOK|wxICON_WARNING|wxCENTRE);
                                            }
                                        }
                                    ++currentColumn;
                                    }
                                ++currentRow;
                                }
                            }
                        }
                    // if there are common axis queued, add them now
                    if (m_commonAxesPlaceholders.size())
                        {
                        for (const auto& commonAxisInfo : m_commonAxesPlaceholders)
                            {
                            std::vector<std::shared_ptr<Graphs::Graph2D>> childGraphs;
                            for (const auto& commonAxisInfo : m_commonAxesPlaceholders)
                                {
                                for (const auto& childId : commonAxisInfo.m_childrenIds)
                                    {
                                    auto childGraph = std::find_if(embeddedGraphs.cbegin(), embeddedGraphs.cend(),
                                        [&childId](const auto& graph) noexcept
                                          {
                                          return graph->GetId() == static_cast<long>(childId);
                                          });
                                    if (childGraph != embeddedGraphs.end() &&
                                        (*childGraph) != nullptr)
                                        { childGraphs.push_back(*childGraph); }
                                    }
                                if (childGraphs.size() > 1)
                                    {
                                    auto commonAxis = (commonAxisInfo.m_axisType == AxisType::BottomXAxis ||
                                                       commonAxisInfo.m_axisType == AxisType::TopXAxis) ?
                                        CommonAxisBuilder::BuildXAxis(canvas,
                                            childGraphs, commonAxisInfo.m_axisType,
                                            commonAxisInfo.m_commonPerpendicularAxis) :
                                        CommonAxisBuilder::BuildYAxis(canvas,
                                            childGraphs, commonAxisInfo.m_axisType);
                                    LoadAxis(commonAxisInfo.m_node, *commonAxis);
                                    LoadItem(commonAxisInfo.m_node, commonAxis);
                                    // force the row to its height and no more
                                    commonAxis->FitCanvasRowHeightToContent(true);
                                    canvas->SetFixedObject(
                                        commonAxisInfo.m_gridPosition.first,
                                        commonAxisInfo.m_gridPosition.second,
                                        commonAxis);
                                    }
                                }
                            }
                        }
                    canvas->CalcRowDimensions();
                    canvas->FitToPageWhenPrinting(true);
                    canvas->SetSizeFromPaperSize();
                    // if multiple rows, then treat it as a report that maintains
                    // the aspect ratio of its content
                    canvas->MaintainAspectRatio(rowCount > 1);
                    reportPages.push_back(canvas);
                    }
                }
            }

        return reportPages;
        }

    //---------------------------------------------------
    std::optional<LabelPlacement> ReportBuilder::ConvertLabelPlacement(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring, LabelPlacement> labelPlacementValues =
            {
            { L"next-to-parent", LabelPlacement::NextToParent },
            { L"flush", LabelPlacement::Flush }
            };

        const auto foundValue = labelPlacementValues.find(value.Lower().ToStdWstring());
        return ((foundValue != labelPlacementValues.cend()) ?
            std::optional<LabelPlacement>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<TextAlignment> ReportBuilder::ConvertTextAlignment(const wxString& value)
        {
        static const std::map<std::wstring, TextAlignment> textAlignValues =
            {
            { L"flush-left", TextAlignment::FlushLeft },
            { L"flush-right", TextAlignment::FlushRight },
            { L"ragged-right", TextAlignment::RaggedRight },
            { L"ragged-left", TextAlignment::RaggedLeft },
            { L"centered", TextAlignment::Centered },
            { L"justified", TextAlignment::Justified }
            };
        
        const auto foundValue = textAlignValues.find(value.Lower().ToStdWstring());
        return ((foundValue != textAlignValues.cend()) ?
            std::optional<TextAlignment>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<wxBrushStyle> ReportBuilder::ConvertBrushStyle(const wxString& value)
        {
        static const std::map<std::wstring, wxBrushStyle> styleValues =
            {
            { L"backwards-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_BDIAGONAL_HATCH },
            { L"forward-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_FDIAGONAL_HATCH },
            { L"cross-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_CROSSDIAG_HATCH },
            { L"solid", wxBrushStyle::wxBRUSHSTYLE_SOLID },
            { L"cross-hatch", wxBrushStyle::wxBRUSHSTYLE_CROSS_HATCH },
            { L"horizontal-hatch", wxBrushStyle::wxBRUSHSTYLE_HORIZONTAL_HATCH },
            { L"vertical-hatch", wxBrushStyle::wxBRUSHSTYLE_VERTICAL_HATCH }
            };

        const auto foundValue = styleValues.find(value.Lower().ToStdWstring());
        return ((foundValue != styleValues.cend()) ?
            std::optional<wxBrushStyle>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<BoxEffect> ReportBuilder::ConvertBoxEffect(const wxString& value)
        {
        static const std::map<std::wstring_view, BoxEffect> boxEffects =
            {
            { L"common-image", BoxEffect::CommonImage },
            { L"fade-from-bottom-to-top", BoxEffect::FadeFromBottomToTop },
            { L"fade-from-left-to-right", BoxEffect::FadeFromLeftToRight },
            { L"fade-from-right-to-left", BoxEffect::FadeFromRightToLeft },
            { L"fade-from-top-to-bottom", BoxEffect::FadeFromTopToBottom },
            { L"glassy", BoxEffect::Glassy },
            { L"solid", BoxEffect::Solid },
            { L"stipple", BoxEffect::Stipple }
            };

        const auto foundValue = boxEffects.find(value.Lower().ToStdWstring());
        return ((foundValue != boxEffects.cend()) ?
            std::optional<BoxEffect>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<Histogram::BinningMethod> ReportBuilder::ConvertBinningMethod(const wxString& value)
        {
        static const std::map<std::wstring, Histogram::BinningMethod> binMethods =
            {
            { L"bin-by-integer-range", Histogram::BinningMethod::BinByIntegerRange },
            { L"bin-by-range", Histogram::BinningMethod::BinByRange },
            { L"bin-unique-values", Histogram::BinningMethod::BinUniqueValues }
            };

        const auto foundValue = binMethods.find(value.Lower().ToStdWstring());
        return ((foundValue != binMethods.cend()) ?
            std::optional<Histogram::BinningMethod>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<Histogram::IntervalDisplay> ReportBuilder::ConvertIntervalDisplay(const wxString& value)
        {
        static const std::map<std::wstring, Histogram::IntervalDisplay> binIntervals =
            {
            { L"cutpoints", Histogram::IntervalDisplay::Cutpoints },
            { L"midpoints", Histogram::IntervalDisplay::Midpoints }
            };

        const auto foundValue = binIntervals.find(value.Lower().ToStdWstring());
        return ((foundValue != binIntervals.cend()) ?
            std::optional<Histogram::IntervalDisplay>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<RoundingMethod> ReportBuilder::ConvertRoundingMethod(const wxString& value)
        {
        static const std::map<std::wstring, RoundingMethod> roundingMethods =
            {
            { L"no-rounding", RoundingMethod::NoRounding },
            { L"round", RoundingMethod::Round },
            { L"round-down", RoundingMethod::RoundDown },
            { L"round-up", RoundingMethod::RoundUp }
            };

        const auto foundValue = roundingMethods.find(value.Lower().ToStdWstring());
        return ((foundValue != roundingMethods.cend()) ?
            std::optional<RoundingMethod>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<BinLabelDisplay> ReportBuilder::ConvertBinLabelDisplay(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring, BinLabelDisplay> bDisplayValues =
            {
            { L"percentage", BinLabelDisplay::BinPercentage },
            { L"value", BinLabelDisplay::BinValue },
            { L"value-and-percentage", BinLabelDisplay::BinValueAndPercentage },
            { L"no-display", BinLabelDisplay::NoDisplay },
            { L"bin-name", BinLabelDisplay::BinName }
            };

        const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
        return ((foundValue != bDisplayValues.cend()) ?
            std::optional<BinLabelDisplay>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<AxisType> ReportBuilder::ConvertAxisType(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring, AxisType> axisValues =
            {
            { L"bottom-x", AxisType::BottomXAxis },
            { L"top-x", AxisType::TopXAxis },
            { L"left-y", AxisType::LeftYAxis },
            { L"right-y", AxisType::RightYAxis }
            };

        const auto foundValue = axisValues.find(value.Lower().ToStdWstring());
        return ((foundValue != axisValues.cend()) ?
            std::optional<AxisType>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBrush(const wxSimpleJSON::Ptr_t& brushNode, wxBrush& brush)
        {
        if (brushNode->IsOk())
            {
            if (brushNode->IsValueNull())
                { brush = wxNullBrush; }
            else
                {
                const wxColour brushColor(
                    ConvertColor(brushNode->GetProperty(L"color")->GetValueString()));
                if (brushColor.IsOk())
                    { brush.SetColour(brushColor); }

                const auto foundStyle = ConvertBrushStyle(brushNode->GetProperty(L"style")->GetValueString());
                if (foundStyle)
                    { brush.SetStyle(foundStyle.value()); }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen)
        {
        static const std::map<std::wstring, wxPenStyle> styleValues =
            {
            { L"dot", wxPenStyle::wxPENSTYLE_DOT },
            { L"dot-dash", wxPenStyle::wxPENSTYLE_DOT_DASH },
            { L"long-dash", wxPenStyle::wxPENSTYLE_LONG_DASH },
            { L"short-dash", wxPenStyle::wxPENSTYLE_SHORT_DASH },
            { L"solid", wxPenStyle::wxPENSTYLE_SOLID },
            { L"cross-hatch", wxPenStyle::wxPENSTYLE_CROSS_HATCH },
            { L"horizontal-hatch", wxPenStyle::wxPENSTYLE_HORIZONTAL_HATCH },
            { L"vertical-hatch", wxPenStyle::wxPENSTYLE_VERTICAL_HATCH }
            };

        if (penNode->IsOk())
            {
            if (penNode->IsValueNull())
                { pen = wxNullPen; }
            else
                {
                const wxColour penColor(
                    ConvertColor(penNode->GetProperty(L"color")->GetValueString()));
                if (penColor.IsOk())
                    { pen.SetColour(penColor); }

                if (penNode->GetProperty(L"width")->IsOk())
                    { pen.SetWidth(penNode->GetProperty(L"width")->GetValueNumber(1)); }

                const auto style = styleValues.find(
                    penNode->GetProperty(L"style")->GetValueString().Lower().ToStdWstring());
                if (style != styleValues.cend())
                    { pen.SetStyle(style->second); }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis)
        {
        static const std::map<std::wstring_view, Axis::TickMark::DisplayType> tickmarkValues =
            {
            { L"inner", Axis::TickMark::DisplayType::Inner },
            { L"outer", Axis::TickMark::DisplayType::Outer },
            { L"crossed", Axis::TickMark::DisplayType::Crossed },
            { L"no-display", Axis::TickMark::DisplayType::NoDisplay }
            };

        static const std::map<std::wstring_view, AxisLabelDisplay> labelDisplayValues =
            {
            { L"custom-labels-or-values", AxisLabelDisplay::DisplayCustomLabelsOrValues },
            { L"only-custom-labels", AxisLabelDisplay::DisplayOnlyCustomLabels },
            { L"custom-labels-and-values", AxisLabelDisplay::DisplayCustomLabelsAndValues },
            { L"no-display", AxisLabelDisplay::NoDisplay }
            };

        static const std::map<std::wstring_view, BracketLineStyle> bracketLineValues =
            {
            { L"arrow", BracketLineStyle::Arrow },
            { L"lines", BracketLineStyle::Lines },
            { L"reverse-arrow", BracketLineStyle::ReverseArrow },
            { L"curly-braces", BracketLineStyle::CurlyBraces },
            { L"no-connection-lines", BracketLineStyle::NoConnectionLines },
            };

        const auto titleProperty = axisNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            auto titleLabel = LoadLabel(titleProperty, GraphItems::Label());
            if (titleLabel != nullptr)
                { axis.GetTitle() = *titleLabel; }
            }
        const auto tickmarksProperty = axisNode->GetProperty(L"tickmarks");
        if (tickmarksProperty->IsOk())
            {
            const auto display = tickmarksProperty->GetProperty(L"display")->GetValueString().Lower();
            auto foundPos = tickmarkValues.find(std::wstring_view(display.wc_str()));
            if (foundPos != tickmarkValues.cend())
                { axis.SetTickMarkDisplay(foundPos->second); }
            }
        const auto display = axisNode->GetProperty(L"label-display")->GetValueString().Lower();
        const auto foundPos = labelDisplayValues.find(std::wstring_view(display.wc_str()));
        if (foundPos != labelDisplayValues.cend())
            { axis.SetLabelDisplay(foundPos->second); }

        axis.SetDoubleSidedAxisLabels(axisNode->GetProperty(L"double-sided-labels")->GetValueBool());

        // pens
        LoadPen(axisNode->GetProperty(L"axis-pen"), axis.GetAxisLinePen());
        LoadPen(axisNode->GetProperty(L"gridline-pen"), axis.GetGridlinePen());

        // brackets
        const auto bracketsNode = axisNode->GetProperty(L"brackets");
        if (bracketsNode->IsOk())
            {
            wxPen bracketPen{ *wxBLACK_PEN };
            LoadPen(bracketsNode->GetProperty(L"pen"), bracketPen);

            const auto foundBracketStyle = bracketLineValues.find(std::wstring_view(
                bracketsNode->GetProperty(L"style")->GetValueString().wc_str()));

            // if loading brackets based on the dataset
            if (bracketsNode->GetProperty(L"dataset")->IsOk())
                {
                const wxString dsName = bracketsNode->GetProperty(L"dataset")->GetValueString();
                const auto foundDataset = m_datasets.find(dsName);
                if (foundDataset == m_datasets.cend() ||
                    foundDataset->second == nullptr)
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"%s: dataset not found for axis brackets."), dsName).ToUTF8());
                    }

                const auto variablesNode = bracketsNode->GetProperty(L"variables");
                if (variablesNode->IsOk())
                    {
                    const auto labelVarName = variablesNode->GetProperty(L"label")->GetValueString();
                    const auto valueVarName = variablesNode->GetProperty(L"value")->GetValueString();

                    axis.AddBrackets(foundDataset->second, labelVarName, valueVarName);
                    if (bracketPen.IsOk())
                        {
                        for (auto& bracket : axis.GetBrackets())
                            { bracket.GetLinePen() = bracketPen; }
                        }
                    if (foundBracketStyle != bracketLineValues.cend())
                        {
                        for (auto& bracket : axis.GetBrackets())
                            { bracket.SetBracketLineStyle(foundBracketStyle->second); }
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        _(L"Variables not defined for brackets").ToUTF8());
                    }
                }
            // or individually defined brackets
            else if (bracketsNode->GetProperty(L"bracket-items")->IsOk())
                {
                auto brackets = bracketsNode->GetProperty(L"bracket-items")->GetValueArrayObject();
                for (const auto& bracket : brackets)
                    {
                    const std::optional<double> axisPos1 =
                        FindAxisPosition(axis, bracket->GetProperty(L"position-1"));
                    const std::optional<double> axisPos2 =
                        FindAxisPosition(axis, bracket->GetProperty(L"position-2"));

                    wxPen pen(*wxBLACK_PEN);
                    LoadPen(bracket->GetProperty(L"pen"), pen);

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        axis.AddBracket(
                            Axis::AxisBracket(axisPos1.value(), axisPos2.value(),
                                safe_divide<double>(axisPos1.value() + axisPos2.value(), 2),
                                bracket->GetProperty(L"label")->GetValueString(),
                                pen,
                                (foundBracketStyle != bracketLineValues.cend()) ?
                                    foundBracketStyle->second : BracketLineStyle::Lines));
                        }
                    }
                }

            if (bracketsNode->GetProperty(L"simplify")->GetValueBool())
                { axis.SimplifyBrackets(); }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                                       const size_t currentRow, const size_t currentColumn)
        {
        const auto axisType = ConvertAxisType(
            commonAxisNode->GetProperty(L"axis-type")->GetValueString());
        if (axisType.has_value())
            {
            m_commonAxesPlaceholders.push_back(
                {
                axisType.value(),
                std::make_pair(currentRow, currentColumn),
                commonAxisNode->GetProperty(L"child-ids")->GetValueArrayNumber(),
                commonAxisNode->GetProperty(L"common-perpendicular-axis")->GetValueBool(),
                commonAxisNode
                });
            }
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label> ReportBuilder::LoadLabel(
        const wxSimpleJSON::Ptr_t& labelNode, const GraphItems::Label labelTemplate)
        {
        if (labelNode->IsOk())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetText(ExpandConstants(labelNode->GetProperty(L"text")->GetValueString()));
            label->GetPen() = wxNullPen;

            const wxColour bgcolor(
                ConvertColor(labelNode->GetProperty(L"background")->GetValueString()));
            if (bgcolor.IsOk())
                { label->SetFontBackgroundColor(bgcolor); }
            const wxColour color(
                ConvertColor(labelNode->GetProperty(L"color")->GetValueString()));
            if (color.IsOk())
                { label->SetFontColor(color); }

            // an image to the left side of it
            const auto leftSideNode = labelNode->GetProperty(L"left-side-image");
            if (leftSideNode->IsOk())
                {
                auto path = leftSideNode->GetProperty(L"path")->GetValueString();
                if (path.length())
                    {
                    if (!wxFileName::FileExists(path))
                        {
                        path = wxFileName(m_configFilePath).GetPathWithSep() + path;
                        if (!wxFileName::FileExists(path))
                            {
                            throw std::runtime_error(
                                wxString::Format(_(L"%s: label side image not found."), path).ToUTF8());
                            }
                        }
                    label->SetLeftSideImage(Image::LoadFile(path));
                    }
                }

            const auto orientation = labelNode->GetProperty(L"orientation")->GetValueString();
            if (orientation.CmpNoCase(L"horizontal") == 0)
                { label->SetTextOrientation(Orientation::Horizontal); }
            else if (orientation.CmpNoCase(L"vertical") == 0)
                { label->SetTextOrientation(Orientation::Vertical); }

            label->SetLineSpacing(labelNode->GetProperty(L"line-spacing")->GetValueNumber(1));

            // font attributes
            if (labelNode->GetProperty(L"bold")->IsOk())
                {
                labelNode->GetProperty(L"bold")->GetValueBool() ?
                    label->GetFont().MakeBold() :
                    label->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                }

            const auto textAlignment = ConvertTextAlignment(
                labelNode->GetProperty(L"text-alignment")->GetValueString());
            if (textAlignment.has_value())
                { label->SetTextAlignment(textAlignment.value()); }

            // header info
            auto headerNode = labelNode->GetProperty(L"header");
            if (headerNode->IsOk())
                {
                label->GetHeaderInfo().Enable(true);
                if (headerNode->GetProperty(L"bold")->IsOk())
                    {
                    headerNode->GetProperty(L"bold")->GetValueBool() ?
                        label->GetHeaderInfo().GetFont().MakeBold() :
                        label->GetHeaderInfo().GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                    }
                const wxColour color(
                    ConvertColor(headerNode->GetProperty(L"color")->GetValueString()));
                if (color.IsOk())
                    { label->GetHeaderInfo().FontColor(color); }

                label->GetHeaderInfo().RelativeScaling(
                    headerNode->GetProperty(L"relative-scaling")->GetValueNumber(1));

                const auto textAlignment = ConvertTextAlignment(
                    headerNode->GetProperty(L"text-alignment")->GetValueString());
                if (textAlignment.has_value())
                    { label->GetHeaderInfo().LabelAlignment(textAlignment.value()); }
                }

            LoadItem(labelNode, label);
            return label;
            }
        return nullptr;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadConstants(const wxSimpleJSON::Ptr_t& constantsNode)
        {
        if (constantsNode->IsOk())
            {
            const auto values = constantsNode->GetValueArrayObject();
            for (const auto& value : values)
                {
                if (value->IsOk())
                    {
                    const wxString vName = value->GetProperty(L"name")->GetValueString();
                    m_values.insert_or_assign(vName,
                        value->GetProperty(L"value")->IsValueString() ?
                        ValuesType(value->GetProperty(L"value")->GetValueString()) :
                        ValuesType(value->GetProperty(L"value")->GetValueNumber()) );
                    }
                }
            }
        }

    //---------------------------------------------------
    std::optional<std::vector<wxString>> ReportBuilder::ExpandColumnSelections(wxString var,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (var.starts_with(L"{{") && var.ends_with(L"}}"))
            { var = var.substr(2, var.length() - 4); }
        else
            { return std::nullopt; }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(everything|matches)" +
                         OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(var))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount < 2)
                { return std::nullopt; }

            const wxString funcName = re.GetMatch(var, 1).MakeLower();
            std::vector<wxString> columns;

            if (funcName.CmpNoCase(L"everything") == 0)
                {
                if (dataset->GetIdColumn().GetName().length())
                    { columns.emplace_back(dataset->GetIdColumn().GetName()); }
                const auto catCols{ dataset->GetCategoricalColumnNames() };
                const auto contCols{ dataset->GetContinuousColumnNames() };
                const auto dateCols{ dataset->GetDateColumnNames() };
                std::copy(catCols.cbegin(), catCols.cend(), std::back_inserter(columns));
                std::copy(contCols.cbegin(), contCols.cend(), std::back_inserter(columns));
                std::copy(dateCols.cbegin(), dateCols.cend(), std::back_inserter(columns));

                return columns;
                }
            else if (paramPartsCount >= 3)
                {
                const wxString columnPattern =
                    ConvertColumnOrGroupParameter(re.GetMatch(var, 2), dataset);
                
                if (funcName.CmpNoCase(L"matches") == 0)
                    {
                    wxRegEx re(columnPattern);
                    if (re.IsValid())
                        {
                        // get columns that contain the string
                        if (re.Matches(dataset->GetIdColumn().GetName()) )
                            { columns.emplace_back(dataset->GetIdColumn().GetName()); }
                        for (const auto& col : dataset->GetCategoricalColumns())
                            {
                            if (re.Matches(col.GetName()))
                                { columns.emplace_back(col.GetName()); }
                            }
                        for (const auto& col : dataset->GetContinuousColumns())
                            {
                            if (re.Matches(col.GetName()))
                                { columns.emplace_back(col.GetName()); }
                            }
                        for (const auto& col : dataset->GetDateColumns())
                            {
                            if (re.Matches(col.GetName()))
                                { columns.emplace_back(col.GetName()); }
                            }
                        return columns;
                        }
                    else
                        { return std::nullopt; }
                    }
                else
                    { return std::nullopt; }
                }
            else
                { return std::nullopt; }
            }
        else
            { return std::nullopt; }
        }

    //---------------------------------------------------
    void ReportBuilder::CalcFormulas(const wxSimpleJSON::Ptr_t& formulasNode,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (formulasNode->IsOk())
            {
            const auto formulas = formulasNode->GetValueArrayObject();
            for (const auto& formula : formulas)
                {
                if (formula->IsOk())
                    {
                    const wxString vName = formula->GetProperty(L"name")->GetValueString();
                    if (formula->GetProperty(L"value")->IsValueString())
                        {
                        m_values.insert_or_assign(vName,
                            CalcFormula(formula->GetProperty(L"value")->GetValueString(), dataset) );
                        }
                    else if (formula->GetProperty(L"value")->IsValueNumber())
                        {
                        m_values.insert_or_assign(vName,
                            formula->GetProperty(L"value")->GetValueNumber() );
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType ReportBuilder::CalcFormula(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        const wxRegEx re(FunctionStartRegEx() +
            L"(min|max|n|total|grandtotal|groupcount|grouppercentdecimal|grouppercent|continuouscolumn)" +
            OpeningParenthesisRegEx());
        if (re.Matches(formula))
            {
            const wxString funcName = re.GetMatch(formula, 1).MakeLower();
            if (funcName.CmpNoCase(L"min") == 0 ||
                funcName.CmpNoCase(L"max") == 0)
                { return CalcMinMax(formula, dataset); }
            else if (funcName.CmpNoCase(L"n") == 0)
                {
                const auto calcValue = CalcValidN(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"total") == 0)
                {
                const auto calcValue = CalcTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"grandtotal") == 0)
                {
                const auto calcValue = CalcGrandTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"groupcount") == 0)
                {
                const auto calcValue = CalcGroupCount(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"grouppercentdecimal") == 0)
                {
                const auto calcValue = CalcGroupPercentDecimal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"grouppercent") == 0)
                {
                const auto calcValue = CalcGroupPercent(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"continuouscolumn") == 0)
                { return ExpandColumnSelection(formula, dataset); }
            }
        // note that formula may just be a constant (e.g., color string),
        // so return that if it can't be calculated into something else
        return formula;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ExpandColumnSelection(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        const wxRegEx re(FunctionStartRegEx() +
            L"(continuouscolumn)" + OpeningParenthesisRegEx() +
            NumberRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString columnIndexStr = re.GetMatch(formula, 2);
                unsigned long columnIndex{ 0 };
                if (columnIndexStr.ToULong(&columnIndex))
                    {
                    if (columnIndex >= dataset->GetContinuousColumns().size())
                        {
                        throw std::runtime_error(
                        wxString::Format(
                            _(L"%lu: invalid continuous column index."), columnIndex).ToUTF8());
                        }
                    return dataset->GetContinuousColumn(columnIndex).GetName();
                    }
                }
            }

        // can't get the name of the column, just return the original text
        return formula;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGroupCount(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                                 L"(groupcount)" + OpeningParenthesisRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSepatatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 4)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString groupName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 3), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."),
                                            groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."),
                                            groupValue).ToUTF8());
                    }

                return dataset->GetCategoricalColumnValidN(groupName, groupName,
                                                           groupID.value());
                }
            // dataset or something missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGroupPercentDecimal(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        
        const wxRegEx re(FunctionStartRegEx() +
                         L"(grouppercentdecimal)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSepatatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            const wxRegEx reFunctionRename(L"(?i)(grouppercentdecimal)");
            if (reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"groupcount");
                const auto groupTotal = CalcGroupCount(countFormula, dataset);
                if (groupTotal)
                    {
                    return safe_divide<double>(groupTotal.value(), dataset->GetRowCount());
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<wxString> ReportBuilder::CalcGroupPercent(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        
        const wxRegEx re(FunctionStartRegEx() +
                         L"(grouppercent)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSepatatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            const wxRegEx reFunctionRename(L"(?i)(grouppercent)");
            if (reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"grouppercentdecimal");
                const auto percDec = CalcGroupPercentDecimal(countFormula, dataset);
                if (percDec)
                    {
                    return wxNumberFormatter::ToString(percDec.value() * 100, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%";
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcValidN(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx reSimple(FunctionStartRegEx() +
                               L"(n)" + OpeningParenthesisRegEx() +
                               ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        const wxRegEx reExtended(FunctionStartRegEx() +
                                 L"(n)" + OpeningParenthesisRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSepatatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSepatatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (reSimple.Matches(formula))
            {
            const auto paramPartsCount = reSimple.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = reSimple.GetMatch(formula, 1).MakeLower();
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reSimple.GetMatch(formula, 2), dataset);
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName);
                    }
                else if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName);
                    }
                }
            // dataset or column name missing
            else
                { return std::nullopt; }
            }
        else if (reExtended.Matches(formula))
            {
            const auto paramPartsCount = reExtended.GetMatchCount();
            if (paramPartsCount >= 5)
                {
                const wxString funcName = reExtended.GetMatch(formula, 1).MakeLower();
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 2), dataset);
                const wxString groupName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 3), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 4), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."),
                                            groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."),
                                            groupValue).ToUTF8());
                    }
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName, groupName,
                                                                        groupID.value());
                    }
                else if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName, groupName,
                                                                    groupID.value());
                    }
                }
            // dataset or something missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ConvertColumnOrGroupParameter(wxString columnStr,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (columnStr.starts_with(L"`") && columnStr.ends_with(L"`"))
            { columnStr = columnStr.substr(1, columnStr.length() - 2); }
        else if (columnStr.starts_with(L"{{") && columnStr.ends_with(L"}}"))
            {
            columnStr = columnStr.substr(2, columnStr.length() - 4);
            columnStr = ExpandConstants(columnStr);
            const auto calcStr = CalcFormula(columnStr, dataset);
            if (const auto strVal{ std::get_if<wxString>(&calcStr) };
                strVal != nullptr)
                { return *strVal; }
            else
                { return wxEmptyString; }
            }

        return columnStr;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGrandTotal(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(grandtotal)" + OpeningParenthesisRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                // only continuous can be totalled
                double total{ 0 };
                for (size_t i = 0; i < dataset->GetContinuousColumns().size(); ++i)
                    { total += dataset->GetContinuousTotal(i); }
                return total;
                }
            // dataset or column name missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcTotal(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(total)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString columnName = ConvertColumnOrGroupParameter(
                                                re.GetMatch(formula, 2), dataset);
                // only continuous can be totalled
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousTotal(columnName);
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                        _(L"%s: column must be continuous when totalling."), columnName).ToUTF8());
                    }
                }
            // dataset or column name missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType ReportBuilder::CalcMinMax(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(min|max)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString columnName = ConvertColumnOrGroupParameter(
                                                re.GetMatch(formula, 2), dataset);

                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    const auto [minVal, maxVal] =
                        dataset->GetCategoricalMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                else if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    const auto [minVal, maxVal] =
                        dataset->GetContinuousMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                }
            // dataset or column name missing
            else
                { return formula; }
            }
        return formula;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadPivots(const wxSimpleJSON::Ptr_t& pivotsNode,
                                   const std::shared_ptr<const Data::Dataset>& parentToPivot)
        {
        if (pivotsNode->IsOk())
            {
            auto pivots = pivotsNode->GetValueArrayObject();
            for (const auto& pivot : pivots)
                {
                if (pivot->IsOk())
                    {
                    Pivot pw;
                    const auto pivotType = pivot->GetProperty(L"type")->GetValueString(L"wider");
                    if (pivotType.CmpNoCase(L"wider") == 0)
                        {
                        auto pivotedData = pw.PivotWider(parentToPivot,
                            pivot->GetProperty(L"id-columns")->GetValueStringVector(),
                            pivot->GetProperty(L"names-from-column")->GetValueString(),
                            pivot->GetProperty(L"values-from-columns")->GetValueStringVector(),
                            pivot->GetProperty(L"names-separator")->GetValueString(L"_"),
                            pivot->GetProperty(L"names-prefix")->GetValueString(),
                            pivot->GetProperty(L"fill-value")->GetValueNumber(
                                std::numeric_limits<double>::quiet_NaN()));

                        if (pivotedData)
                            {
                            m_datasets.insert_or_assign(
                                pivot->GetProperty(L"name")->GetValueString(), pivotedData);
                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else if (pivotType.CmpNoCase(L"longer") == 0)
                        {
                        auto pivotedData = pw.PivotLonger(parentToPivot,
                            pivot->GetProperty(L"columns-to-keep")->GetValueStringVector(),
                            pivot->GetProperty(L"from-columns")->GetValueStringVector(),
                            pivot->GetProperty(L"names-to")->GetValueStringVector(),
                            pivot->GetProperty(L"values-to")->GetValueString(),
                            pivot->GetProperty(L"names-pattern")->GetValueString());

                        if (pivotedData)
                            {
                            m_datasets.insert_or_assign(
                                pivot->GetProperty(L"name")->GetValueString(), pivotedData);
                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: unrecognized pivot method."), pivotType).ToUTF8());
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadSubsets(const wxSimpleJSON::Ptr_t& subsetsNode,
                                    const std::shared_ptr<const Data::Dataset>& parentToSubset)
        {
        static const std::map<std::wstring_view, Comparison> cmpOperators =
            {
            { L"=", Comparison::Equals },
            { L"==", Comparison::Equals },
            { L"!=", Comparison::NotEquals },
            { L"<>", Comparison::NotEquals },
            { L"<", Comparison::LessThan },
            { L"<=", Comparison::LessThanOrEqualTo },
            { L">", Comparison::GreaterThan },
            { L">=", Comparison::GreaterThanOrEqualTo }
            };

        const auto loadColumnFilter = [this](const auto& filterNode)
            {
            const auto foundPos = cmpOperators.find(std::wstring_view(
                filterNode->GetProperty(L"operator")->
                GetValueString().MakeLower().wc_str()));
            Comparison cmp = (foundPos != cmpOperators.cend() ?
                foundPos->second : Comparison::Equals);
                            
            const auto valueNode = filterNode->GetProperty(L"value");
                            
            if (valueNode->IsOk())
                {
                wxDateTime dt;
                const bool isDate =
                    (valueNode->IsValueString() &&
                        (dt.ParseDateTime(valueNode->GetValueString()) ||
                         dt.ParseDate(valueNode->GetValueString())));
                ColumnFilterInfo cFilter 
                    {
                    filterNode->GetProperty(L"column")->GetValueString(),
                    cmp,
                    (isDate ?
                        DatasetValueType(dt) :
                        valueNode->IsValueString() ?
                        DatasetValueType(ExpandConstants(valueNode->GetValueString())) :
                        DatasetValueType(valueNode->GetValueNumber()))
                    };
                return cFilter;
                }
            else
                {
                throw std::runtime_error(
                    _(L"Comparison value for subset filter missing.").ToUTF8());
                }
            };

        if (subsetsNode->IsOk())
            {
            auto subsets = subsetsNode->GetValueArrayObject();
            for (const auto& subset : subsets)
                {
                if (subset->IsOk())
                    {
                    const auto filterNode = subset->GetProperty(L"filter");
                    const auto filterAndNode = subset->GetProperty(L"filter-and");
                    const auto filterOrNode = subset->GetProperty(L"filter-or");
                    const auto validFilterTypeNodes =
                        (filterNode->IsOk() ? 1 : 0) +
                        (filterAndNode->IsOk() ? 1 : 0) +
                        (filterOrNode->IsOk() ? 1 : 0);
                    if (validFilterTypeNodes  > 1)
                        {
                        throw std::runtime_error(
                            _(L"Only one filter type allowed for a subset.").ToUTF8());
                        }
                    else if (validFilterTypeNodes == 0)
                        {
                        throw std::runtime_error(
                            _(L"Subset missing filters.").ToUTF8());
                        }

                    Subset dataSubsetter;
                    std::shared_ptr<Data::Dataset> subsettedDataset{ nullptr };
                    // single column filter
                    if (filterNode->IsOk())
                        {
                        subsettedDataset = dataSubsetter.SubsetSimple(
                                parentToSubset, loadColumnFilter(filterNode));
                        }
                    // ANDed filters
                    else if (filterAndNode->IsOk())
                        {
                        std::vector<ColumnFilterInfo> cf;
                        const auto filterNodes = filterAndNode->GetValueArrayObject();
                        if (filterNodes.size() == 0)
                            {
                            throw std::runtime_error(
                                _(L"Subset missing filters.").ToUTF8());
                            }
                        for (const auto& filterNode : filterNodes)
                            { cf.emplace_back(loadColumnFilter(filterNode)); }
                            
                        subsettedDataset = dataSubsetter.SubsetAnd(parentToSubset, cf);
                        }
                    // ORed filters
                    else if (filterOrNode->IsOk())
                        {
                        std::vector<ColumnFilterInfo> cf;
                        const auto filterNodes = filterOrNode->GetValueArrayObject();
                        if (filterNodes.size() == 0)
                            {
                            throw std::runtime_error(
                                _(L"Subset missing filters.").ToUTF8());
                            }
                        for (const auto& filterNode : filterNodes)
                            { cf.emplace_back(loadColumnFilter(filterNode)); }

                        subsettedDataset = dataSubsetter.SubsetOr(parentToSubset, cf);
                        }

                    if (subsettedDataset)
                        {
                        m_datasets.insert_or_assign(
                            subset->GetProperty(L"name")->GetValueString(), subsettedDataset);
                        LoadDatasetTransformations(subset, subsettedDataset);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasetTransformations(const wxSimpleJSON::Ptr_t& dsNode,
        std::shared_ptr<Data::Dataset>& dataset)
        {
        if (dsNode->IsOk())
            {
            // column renaming
            auto colRenames = dsNode->GetProperty(L"columns-rename")->GetValueArrayObject();
            for (const auto& colRename : colRenames)
                {
                dataset->RenameColumn(
                    colRename->GetProperty(L"name")->GetValueString(),
                    colRename->GetProperty(L"new-name")->GetValueString());
                }
            // label recoding
            auto recodeREs = dsNode->GetProperty(L"recode-re")->GetValueArrayObject();
            for (const auto& recodeRE : recodeREs)
                {
                dataset->RecodeRE(
                    recodeRE->GetProperty(L"column")->GetValueString(),
                    recodeRE->GetProperty(L"pattern")->GetValueString(),
                    recodeRE->GetProperty(L"replacement")->GetValueString());
                }

            // load any constants defined with this dataset
            CalcFormulas(dsNode->GetProperty(L"formulas"), dataset);

            // load any subsets of this dataset
            LoadSubsets(dsNode->GetProperty(L"subsets"), dataset);

            // load any pivots of this dataset
            LoadPivots(dsNode->GetProperty(L"pivots"), dataset);

            const auto exportPath =
                    dsNode->GetProperty(L"export-path")->GetValueString();
            // A project silently writing to an arbitrary file is
            // a security threat vector, so only allow that for builds
            // with DEBUG_FILE_IO explicitly set.
            // This should only be used for reviewing the output from a pivot operation
            // when designing a project (in release build).
            if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::AllowFileIO))
                {
                if (exportPath.length())
                    {
                    wxFileName fn(exportPath);
                    if (fn.GetPath().empty())
                        { fn = wxFileName(m_configFilePath).GetPathWithSep() + exportPath; }                     
                    if (fn.GetExt().CmpNoCase(L"csv") == 0)
                        { dataset->ExportCSV(fn.GetFullPath()); }
                    else
                        { dataset->ExportTSV(fn.GetFullPath()); }
                    }
                }
            else if (exportPath.length())
                {
                // just log this (don't throw)
                wxLogWarning(
                        wxString::Format(L"Dataset '%s' cannot be exported "
                            "because debug file IO is not enabled.",
                            dataset->GetName()));
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasets(const wxSimpleJSON::Ptr_t& datasetsNode)
        {
        if (datasetsNode->IsOk())
            {
            auto datasets = datasetsNode->GetValueArrayObject();
            for (const auto& datasetNode : datasets)
                {
                if (datasetNode->IsOk())
                    {
                    const wxString dsName = datasetNode->GetProperty(L"name")->GetValueString();
                    wxString path = datasetNode->GetProperty(L"path")->GetValueString();
                    const wxString importer = datasetNode->GetProperty(L"importer")->GetValueString();
                    // read the variables info
                    //------------------------
                    // ID column
                    const wxString idColumn = datasetNode->GetProperty(L"id-column")->GetValueString();
                    // date columns
                    std::vector<Data::ImportInfo::DateImportInfo> dateInfo;
                    const auto dateProperty = datasetNode->GetProperty(L"date-columns");
                    if (dateProperty->IsOk())
                        {
                        const auto dateVars = dateProperty->GetValueArrayObject();
                        for (const auto& dateVar : dateVars)
                            {
                            if (dateVar->IsOk())
                                {
                                // get the date column's name and how to load it
                                const wxString dateName =
                                    dateVar->GetProperty(L"name")->GetValueString();
                                if (dateName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Date column must have a name.")).ToUTF8());
                                    }
                                const wxString dateParser =
                                    dateVar->GetProperty(L"parser")->GetValueString();
                                const wxString dateFormat =
                                    dateVar->GetProperty(L"format")->GetValueString();
                                dateInfo.push_back(
                                    {
                                    dateName,
                                    (dateParser.CmpNoCase(L"iso-date") == 0 ?
                                        DateImportMethod::IsoDate :
                                     dateParser.CmpNoCase(L"iso-combined") == 0 ?
                                        DateImportMethod::IsoCombined :
                                     dateParser.CmpNoCase(L"strptime-format") == 0 ?
                                        DateImportMethod::StrptimeFormatString :
                                     dateParser.CmpNoCase(L"rfc822") == 0 ?
                                        DateImportMethod::Rfc822 :
                                     DateImportMethod::Automatic),
                                    dateFormat
                                    }
                                    );
                                }
                            }
                        }
                    // continuous columns
                    const std::vector<wxString> continuousVars =
                        datasetNode->GetProperty(L"continuous-columns")->GetValueStringVector();
                    // categorical columns
                    std::vector<Data::ImportInfo::CategoricalImportInfo> catInfo;
                    const auto catProperty = datasetNode->GetProperty(L"categorical-columns");
                    if (catProperty->IsOk())
                        {
                        const auto catVars = catProperty->GetValueArrayObject();
                        for (const auto& catVar : catVars)
                            {
                            if (catVar->IsOk())
                                {
                                // get the cat column's name and how to load it
                                const wxString catName = catVar->GetProperty(L"name")->GetValueString();
                                if (catName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Categorical column must have a name.")).ToUTF8());
                                    }
                                const wxString catParser =
                                    catVar->GetProperty(L"parser")->GetValueString();
                                catInfo.push_back(
                                    {
                                    catName,
                                    (catParser.CmpNoCase(L"as-integers") == 0 ?
                                      CategoricalImportMethod::ReadAsIntegers :
                                      CategoricalImportMethod::ReadAsStrings)
                                    }
                                    );
                                }
                            }
                        }

                    // create the dataset
                    auto dataset = std::make_shared<Data::Dataset>();
                    // validate settings
                    if (dsName.empty())
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a name.")).ToUTF8());
                        }
                    if (path.empty())
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a filepath.")).ToUTF8());
                        }
                    if (!wxFileName::FileExists(path))
                        {
                        path = wxFileName(m_configFilePath).GetPathWithSep() + path;
                        if (!wxFileName::FileExists(path))
                            {
                            throw std::runtime_error(
                                wxString(_(L"Dataset not found.")).ToUTF8());
                            }
                        }
                    // import using the user-provided parser or deduce from the file extension
                    const auto fileExt(wxFileName(path).GetExt());                        
                    if (importer.CmpNoCase(L"csv") == 0 ||
                        fileExt.CmpNoCase(L"csv") == 0)
                        {
                        dataset->ImportCSV(path,
                            ImportInfo().
                            IdColumn(idColumn).
                            DateColumns(dateInfo).
                            ContinuousColumns(continuousVars).
                            CategoricalColumns(catInfo).
                            ContinousMDRecodeValue(
                                datasetNode->GetProperty(L"continuous-md-recode-value")->
                                    GetValueNumber(std::numeric_limits<double>::quiet_NaN())));
                        }
                    else if (importer.CmpNoCase(L"tsv") == 0 ||
                        fileExt.CmpNoCase(L"tsv") == 0 ||
                        fileExt.CmpNoCase(L"txt") == 0)
                        {
                        dataset->ImportTSV(path,
                            ImportInfo().
                            IdColumn(idColumn).
                            DateColumns(dateInfo).
                            ContinuousColumns(continuousVars).
                            CategoricalColumns(catInfo).
                            ContinousMDRecodeValue(
                                datasetNode->GetProperty(L"continuous-md-recode-value")->
                                    GetValueNumber(std::numeric_limits<double>::quiet_NaN())));
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a valid importer specified.")).
                                    ToUTF8());
                        }
                    
                    m_datasets.insert_or_assign(dsName, dataset);
                    // recode values, build subsets and pivots, etc.
                    LoadDatasetTransformations(datasetNode, dataset);
                    }
                }
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadLinePlot(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for line plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarName = variablesNode->GetProperty(L"group")->GetValueString();

            auto linePlot = std::make_shared<LinePlot>(canvas,
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            linePlot->SetData(foundPos->second,
                variablesNode->GetProperty(L"y")->GetValueString(),
                variablesNode->GetProperty(L"x")->GetValueString(),
                (groupVarName.length() ? std::optional<wxString>(groupVarName) : std::nullopt));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            return linePlot;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for line plot.").ToUTF8());
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBarChart(const wxSimpleJSON::Ptr_t& graphNode,
        std::shared_ptr<Graphs::BarChart> barChart)
        {
        const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->GetValueString();
        if (bOrientation.CmpNoCase(L"horizontal") == 0)
            { barChart->SetBarOrientation(Orientation::Horizontal); }
        else if (bOrientation.CmpNoCase(L"vertical") == 0)
            { barChart->SetBarOrientation(Orientation::Vertical); }

        const auto boxEffect = ConvertBoxEffect(
                graphNode->GetProperty(L"box-effect")->GetValueString());
        if (boxEffect)
            { barChart->SetBarEffect(boxEffect.value()); }
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::FillableShape>
        ReportBuilder::LoadFillableShape(const wxSimpleJSON::Ptr_t& shapeNode)
        {
        const auto loadedShape = ConvertIcon(shapeNode->GetProperty(L"icon")->GetValueString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: unknown icon for fillable shape."),
                    shapeNode->GetProperty(L"icon")->GetValueString()).ToUTF8());
            }
        
        wxSize sz(32, 32);
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->GetValueNumber(32);
            sz.y = sizeNode->GetProperty(L"height")->GetValueNumber(32);
            }

        wxPen pen(*wxBLACK_PEN);
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(*wxBLACK_BRUSH);
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        double fillPercent{ math_constants::empty };
        const auto fillPercentNode = shapeNode->GetProperty(L"fill-percent");
        if (fillPercentNode->IsOk())
            {
            if (fillPercentNode->IsValueNumber())
                {
                fillPercent = fillPercentNode->GetValueNumber(fillPercent);
                }
            else if (fillPercentNode->IsValueString())
                {
                const auto numberVal = ExpandNumericConstant(fillPercentNode->GetValueString());
                if (numberVal)
                    { fillPercent = numberVal.value(); }
                }
            }

        auto sh = std::make_shared<FillableShape>(
            GraphItemInfo().Anchoring(Anchoring::TopLeftCorner).
            Pen(pen).Brush(brush),
            loadedShape.value(), sz, fillPercent);
        // center by default, but allow LoadItems (below) to override that
        // if client asked for something else
        sh->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        sh->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

        LoadItem(shapeNode, sh);

        // fit the column area to this shape
        sh->SetFixedWidthOnCanvas(true);
        return sh;
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Shape> ReportBuilder::LoadShape(const wxSimpleJSON::Ptr_t& shapeNode)
        {
        const auto loadedShape = ConvertIcon(shapeNode->GetProperty(L"icon")->GetValueString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: unknown icon for shape."),
                    shapeNode->GetProperty(L"icon")->GetValueString()).ToUTF8());
            }
        
        wxSize sz(32, 32);
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->GetValueNumber(32);
            sz.y = sizeNode->GetProperty(L"height")->GetValueNumber(32);
            }

        wxPen pen(*wxBLACK_PEN);
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(*wxWHITE_BRUSH);
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        auto sh = std::make_shared<Shape>(
            GraphItemInfo().Anchoring(Anchoring::TopLeftCorner).
            Pen(pen).Brush(brush),
            loadedShape.value(), sz);
        // center by default, but allow LoadItems (below) to override that
        // if client asked for something else
        sh->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        sh->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

        LoadItem(shapeNode, sh);

        // fit the column area to this shape
        sh->SetFixedWidthOnCanvas(true);
        return sh;
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadHistogram(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for histogram."),
                                 dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto contVarName = variablesNode->GetProperty(L"aggregate")->GetValueString();
            const auto groupName = variablesNode->GetProperty(L"group")->GetValueString();

            const auto binMethod = ConvertBinningMethod(
                graphNode->GetProperty(L"binning-method")->GetValueString());

            const auto binIntervalDisplay = ConvertIntervalDisplay(
                graphNode->GetProperty(L"interval-display")->GetValueString());

            const auto binLabel = ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->GetValueString());

            const auto rounding = ConvertRoundingMethod(
                graphNode->GetProperty(L"rounding")->GetValueString());

            const std::optional<double> startBinsValue = graphNode->GetProperty(L"bins-start")->IsOk() ?
                std::optional<double>(graphNode->GetProperty(L"bins-start")->GetValueNumber()) :
                std::nullopt;
            const std::optional<size_t> suggestedBinCount = graphNode->GetProperty(L"suggested-bin-count")->IsOk() ?
                std::optional<double>(graphNode->GetProperty(L"suggested-bin-count")->GetValueNumber()) :
                std::nullopt;
            const std::optional<size_t> maxBinCount = graphNode->GetProperty(L"max-bin-count")->IsOk() ?
                std::optional<double>(graphNode->GetProperty(L"max-bin-count")->GetValueNumber()) :
                std::nullopt;

            auto histo = std::make_shared<Histogram>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")) );
                
            LoadBarChart(graphNode, histo);

            histo->SetData(foundPos->second, contVarName,
                (groupName.length() ? std::optional<wxString>(groupName) : std::nullopt),
                binMethod.value_or(Histogram::BinningMethod::BinByIntegerRange),
                rounding.value_or(RoundingMethod::NoRounding),
                binIntervalDisplay.value_or(Histogram::IntervalDisplay::Cutpoints),
                binLabel.value_or(BinLabelDisplay::BinValue),
                graphNode->GetProperty(L"show-full-range")->GetValueBool(true),
                startBinsValue, std::make_pair(suggestedBinCount, maxBinCount) );

            LoadGraph(graphNode, canvas, currentRow, currentColumn, histo);
            return histo;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for histogram.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadCategoricalBarChart(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bar chart."),
                                 dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarName = variablesNode->GetProperty(L"aggregate")->GetValueString();
            const auto groupName = variablesNode->GetProperty(L"group")->GetValueString();
            const auto categoryName = variablesNode->GetProperty(L"category")->GetValueString();
            const auto binLabel = ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->GetValueString());

            auto barChart = std::make_shared<CategoricalBarChart>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")) );
                
            LoadBarChart(graphNode, barChart);

            barChart->SetData(foundPos->second, categoryName,
                (aggVarName.length() ? std::optional<wxString>(aggVarName) : std::nullopt),
                (groupName.length() ? std::optional<wxString>(groupName) : std::nullopt),
                binLabel.has_value() ? binLabel.value() : BinLabelDisplay::BinValue);

            LoadGraph(graphNode, canvas, currentRow, currentColumn, barChart);
            return barChart;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for bar chart.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadPieChart(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for pie chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarName = variablesNode->GetProperty(L"aggregate")->GetValueString();
            const auto groupVar1Name = variablesNode->GetProperty(L"group-1")->GetValueString();
            const auto groupVar2Name = variablesNode->GetProperty(L"group-2")->GetValueString();

            auto pieChart = std::make_shared<PieChart>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            pieChart->SetData(foundPos->second,
                (aggVarName.length() ? std::optional<wxString>(aggVarName) : std::nullopt),
                groupVar1Name,
                (groupVar2Name.length() ?std::optional<wxString>(groupVar2Name) : std::nullopt));

            const auto labelPlacement =
                ConvertLabelPlacement(graphNode->GetProperty(L"label-placement")->GetValueString());
            if (labelPlacement.has_value())
                { pieChart->SetLabelPlacement(labelPlacement.value()); }

            const auto outerPieMidLabel = ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-pie-midpoint-label-display")->GetValueString());
            if (outerPieMidLabel.has_value())
                { pieChart->SetOuterPieMidPointLabelDisplay(outerPieMidLabel.value()); }

            const auto innerPieMidLabel = ConvertBinLabelDisplay(
                graphNode->GetProperty(L"inner-pie-midpoint-label-display")->GetValueString());
            if (innerPieMidLabel.has_value())
                { pieChart->SetInnerPieMidPointLabelDisplay(innerPieMidLabel.value()); }

            if (graphNode->HasProperty(L"include-inner-pie-labels"))
                {
                pieChart->ShowInnerPieLabels(
                    graphNode->GetProperty(L"include-inner-pie-labels")->GetValueBool());
                }

            if (graphNode->HasProperty(L"include-outer-pie-labels"))
                {
                pieChart->ShowOuterPieLabels(
                    graphNode->GetProperty(L"include-outer-pie-labels")->GetValueBool());
                }

            if (graphNode->HasProperty(L"color-labels"))
                {
                pieChart->UseColorLabels(
                    graphNode->GetProperty(L"color-labels")->GetValueBool());
                }

            // showcase the slices
            const auto showcaseGroupsNode = graphNode->GetProperty(L"showcase-slices-groups");
            if (showcaseGroupsNode->IsOk())
                {
                pieChart->ShowcaseOuterPieSlicesAndChildren(
                    showcaseGroupsNode->GetValueStringVector());
                }
            const auto showcaseNode = graphNode->GetProperty(L"showcase-slices");
            if (showcaseNode->IsOk())
                {
                const auto pieType = showcaseNode->GetProperty(L"pie")->GetValueString();
                const auto categoryType = showcaseNode->GetProperty(L"category")->GetValueString();
                if (pieType.CmpNoCase(L"inner") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        {
                        pieChart->ShowcaseSmallestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->GetValueBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-labels")->GetValueBool() );
                        }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        {
                        pieChart->ShowcaseLargestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->GetValueBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-labels")->GetValueBool() );
                        }
                    }
                if (pieType.CmpNoCase(L"outer") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        { pieChart->ShowcaseSmallestOuterPieSlices(); }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        { pieChart->ShowcaseLargestOuterPieSlices(); }
                    }
                }

            // donut hole info
            const auto donutHoleNode = graphNode->GetProperty(L"donut-hole");
            if (donutHoleNode->IsOk())
                {
                pieChart->IncludeDonutHole(true);
                const auto labelProperty = donutHoleNode->GetProperty(L"label");
                if (labelProperty->IsOk())
                    {
                    auto holeLabel = LoadLabel(labelProperty, pieChart->GetDonutHoleLabel());
                    if (holeLabel != nullptr)
                        { pieChart->GetDonutHoleLabel() = *holeLabel; }
                    }
                const auto propNode = donutHoleNode->GetProperty(L"proportion");
                if (propNode->IsOk())
                    { pieChart->SetDonutHoleProportion(propNode->GetValueNumber()); }
                const wxColour color(
                    ConvertColor(donutHoleNode->GetProperty(L"color")->GetValueString()));
                if (color.IsOk())
                    { pieChart->SetDonutHoleColor(color); }
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pieChart);
            return pieChart;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for pie chart.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadTable(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for table."), dsName).ToUTF8());
            }

        std::vector<wxString> variables;
        const auto readVariables = graphNode->GetProperty(L"variables")->GetValueStringVector();
        for (const auto& readVar : readVariables)
            {
            auto convertedVars = ExpandColumnSelections(readVar, foundPos->second);
            if (convertedVars)
                {
                variables.insert(variables.cend(),
                    convertedVars.value().cbegin(), convertedVars.value().cend());
                }
            else
                { variables.push_back(readVar); }
            }

        auto table = std::make_shared<Graphs::Table>(canvas);
        table->SetData(foundPos->second, variables,
            graphNode->GetProperty(L"transpose")->GetValueBool());

        const auto minWidthProp = graphNode->GetProperty(L"min-width-proportion");
        if (minWidthProp->IsOk())
            { table->SetMinWidthProportion(minWidthProp->GetValueNumber()); }
        const auto minHeightProp = graphNode->GetProperty(L"min-height-proportion");
        if (minHeightProp->IsOk())
            { table->SetMinHeightProportion(minHeightProp->GetValueNumber()); }

        LoadPen(graphNode->GetProperty(L"highlight-pen"), table->GetHighlightPen());

        const size_t originalColumnCount = table->GetColumnCount();
        const size_t originalRowCount = table->GetRowCount();

        // loads the positions from a row or column stops array
        const auto loadStops = [this, &originalColumnCount, &originalRowCount, &table](const auto& stopsNode)
            {
            std::set<size_t> rowOrColumnStops;
            const auto stops = stopsNode->GetValueArrayObject();
            if (stops.size())
                {
                for (const auto& stop : stops)
                    {
                    const std::optional<size_t> stopPosition =
                        LoadTablePosition(stop->GetProperty(L"position"),
                            originalColumnCount, originalRowCount, table);
                    if (stopPosition.has_value())
                        { rowOrColumnStops.insert(stopPosition.value()); }
                    }
                }
            return rowOrColumnStops;
            };

        // add rows
        auto rowAddCommands = graphNode->GetProperty(L"row-add")->GetValueArrayObject();
        if (rowAddCommands.size())
            {
            for (const auto& rowAddCommand : rowAddCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(rowAddCommand->GetProperty(L"position"),
                        originalColumnCount, originalRowCount, table);
                if (!position.has_value())
                    { continue; }
                table->InsertRow(position.value());
                // fill the values across the row
                const auto values = rowAddCommand->GetProperty(L"values")->GetValueStringVector();
                for (size_t i = 0; i < values.size(); ++i)
                    { table->GetCell(position.value(), i).SetValue(values[i]); }
                const wxColour bgcolor(
                    ConvertColor(rowAddCommand->GetProperty(L"background")->GetValueString()));
                if (bgcolor.IsOk())
                    { table->SetRowBackgroundColor(position.value(), bgcolor, std::nullopt); }
                }
            }
        // group the rows
        const auto rowGroupings = graphNode->GetProperty(L"row-group")->GetValueArrayNumber();
        for (const auto& rowGrouping : rowGroupings)
            { table->GroupRow(rowGrouping); }

        // color the rows
        const auto rowColorCommands = graphNode->GetProperty(L"row-color")->GetValueArrayObject();
        if (rowColorCommands.size())
            {
            for (const auto& rowColorCommand : rowColorCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(rowColorCommand->GetProperty(L"position"),
                        originalColumnCount, originalRowCount, table);
                const wxColour bgcolor(
                    ConvertColor(rowColorCommand->GetProperty(L"background")->GetValueString()));
                const std::set<size_t> colStops =
                    loadStops(rowColorCommand->GetProperty(L"stops"));
                if (position.has_value() && bgcolor.IsOk())
                    {
                    table->SetRowBackgroundColor(position.value(), bgcolor, colStops);
                    }
                }
            }

        // change rows' content alignent
        const auto rowContentCommands =
            graphNode->GetProperty(L"row-content-align")->GetValueArrayObject();
        if (rowContentCommands.size())
            {
            for (const auto& rowContentCommand : rowContentCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(rowContentCommand->GetProperty(L"position"),
                        originalColumnCount, originalRowCount, table);
                if (!position.has_value())
                    { continue; }
                const auto hPageAlignment =
                    rowContentCommand->GetProperty(L"horizontal-page-alignment")->GetValueString();
                const std::set<size_t> colStops =
                    loadStops(rowContentCommand->GetProperty(L"stops"));
                if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                    {
                    table->SetRowHorizontalPageAlignment(position.value(),
                        PageHorizontalAlignment::LeftAligned, colStops);
                    }
                else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                    {
                    table->SetRowHorizontalPageAlignment(position.value(),
                        PageHorizontalAlignment::RightAligned, colStops);
                    }
                else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                    {
                    table->SetRowHorizontalPageAlignment(position.value(),
                        PageHorizontalAlignment::Centered, colStops);
                    }
                }
            }

        // group the columns
        const auto columnGroupings = graphNode->GetProperty(L"column-group")->GetValueArrayNumber();
        for (const auto& columnGrouping : columnGroupings)
            { table->GroupColumn(columnGrouping); }

        // color the columns
        const auto colColorCommands = graphNode->GetProperty(L"column-color")->GetValueArrayObject();
        if (colColorCommands.size())
            {
            for (const auto& colColorCommand : colColorCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(colColorCommand->GetProperty(L"position"),
                        originalColumnCount, originalRowCount, table);
                const wxColour bgcolor(
                    ConvertColor(colColorCommand->GetProperty(L"background")->GetValueString()));
                const std::set<size_t> rowStops =
                    loadStops(colColorCommand->GetProperty(L"stops"));
                if (position.has_value() && bgcolor.IsOk())
                    { table->SetColumnBackgroundColor(position.value(), bgcolor, rowStops); }
                }
            }

        // change columns' borders
        const auto columnBordersCommands =
            graphNode->GetProperty(L"column-borders")->GetValueArrayObject();
        if (columnBordersCommands.size())
            {
            for (const auto& columnBordersCommand : columnBordersCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(columnBordersCommand->GetProperty(L"position"),
                        originalColumnCount, originalRowCount, table);
                const auto borderFlags =
                    columnBordersCommand->GetProperty(L"borders")->GetValueArrayBool();

                const std::set<size_t> rowStops =
                    loadStops(columnBordersCommand->GetProperty(L"stops"));
                if (position.has_value() && borderFlags.size() > 0)
                    {
                    table->SetColumnBorders(position.value(),
                        (borderFlags.size() > 0 ? borderFlags[0] : true),
                        (borderFlags.size() > 1 ? borderFlags[1] : true),
                        (borderFlags.size() > 2 ? borderFlags[2] : true),
                        (borderFlags.size() > 3 ? borderFlags[3] : true),
                        rowStops);
                    }
                }
            }

        // highlight cells down a column
        const auto columnHighlightsCommands =
            graphNode->GetProperty(L"column-highlight")->GetValueArrayObject();
        if (columnHighlightsCommands.size())
            {
            for (const auto& columnHighlightsCommand : columnHighlightsCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(columnHighlightsCommand->GetProperty(L"position"),
                        originalColumnCount, originalRowCount, table);

                std::set<size_t> rowStops =
                    loadStops(columnHighlightsCommand->GetProperty(L"stops"));
                if (position.has_value())
                    { table->HighlightColumn(position.value(), rowStops); }
                }
            }

        // column/row aggregates
        const auto columnAggregates =
            graphNode->GetProperty(L"aggregates")->GetValueArrayObject();
        if (columnAggregates.size())
            {
            for (const auto& columnAggregate : columnAggregates)
                {
                const auto aggName = columnAggregate->GetProperty(L"name")->GetValueString();
                const auto whereType = columnAggregate->GetProperty(L"type")->GetValueString();
                const auto aggType = columnAggregate->GetProperty(L"aggregate-type")->GetValueString();

                // starting column/row
                const std::optional<size_t> startColumn =
                    LoadTablePosition(columnAggregate->GetProperty(L"start"),
                        originalColumnCount, originalRowCount, table);
                // ending column/row
                const std::optional<size_t> endingColumn =
                    LoadTablePosition(columnAggregate->GetProperty(L"end"),
                        originalColumnCount, originalRowCount, table);

                const wxColour bkColor(
                    ConvertColor(columnAggregate->GetProperty(L"background")->GetValueString()));

                Table::AggregateInfo aggInfo;
                if (startColumn.has_value())
                    { aggInfo.FirstCell(startColumn.value()); }
                if (endingColumn.has_value())
                    { aggInfo.LastCell(endingColumn.value()); }

                if (aggType.CmpNoCase(L"percent-change") == 0)
                    { aggInfo.Type(Table::AggregateType::ChangePercent); }
                else if (aggType.CmpNoCase(L"total") == 0)
                    { aggInfo.Type(Table::AggregateType::Total); }
                else if (aggType.CmpNoCase(L"ratio") == 0)
                    { aggInfo.Type(Table::AggregateType::Ratio); }
                // invalid agg column type
                else
                    { continue; }

                if (whereType.CmpNoCase(L"column") == 0)
                    {
                    table->InsertAggregateColumn(aggInfo, aggName,
                        std::nullopt, (bkColor.IsOk() ?
                                       std::optional<wxColour>(bkColor): std::nullopt));
                    }
                else if (whereType.CmpNoCase(L"row") == 0)
                    {
                    table->InsertAggregateRow(aggInfo, aggName,
                        std::nullopt, (bkColor.IsOk() ?
                                       std::optional<wxColour>(bkColor) : std::nullopt));
                    }
                }
            }

        // row totals
        const auto rowTotals =
            graphNode->GetProperty(L"row-totals");
        if (rowTotals->IsOk())
            {
            const wxColour bkColor(
                    ConvertColor(rowTotals->GetProperty(L"background")->GetValueString()));
            table->InsertRowTotals(bkColor.IsOk() ?
                                    std::optional<wxColour>(bkColor) : std::nullopt);
            }

        // cell updating
        const auto cellUpdates =
            graphNode->GetProperty(L"cell-update")->GetValueArrayObject();
        if (cellUpdates.size())
            {
            for (const auto& cellUpdate : cellUpdates)
                {
                // last column and row will be the last aggregates at this point
                // (if applicable)
                const std::optional<size_t> rowPosition =
                    LoadTablePosition(cellUpdate->GetProperty(L"row"),
                        table->GetColumnCount(),
                        table->GetRowCount(), table);
                const std::optional<size_t> columnPosition =
                    LoadTablePosition(cellUpdate->GetProperty(L"column"),
                        table->GetColumnCount(),
                        table->GetRowCount(), table);
                Table::TableCell* currentCell =
                    ((rowPosition.has_value() && columnPosition.has_value() &&
                      rowPosition.value() < table->GetRowCount() &&
                      columnPosition.value() < table->GetColumnCount()) ?
                    &table->GetCell(rowPosition.value(), columnPosition.value()) :
                    table->FindCell(cellUpdate->GetProperty(L"value-to-find")->GetValueString()) );

                if (currentCell)
                    {
                    // column count
                    const auto columnCountProperty =
                        cellUpdate->GetProperty(L"column-count");
                    if (columnCountProperty->IsOk())
                        {
                        if (columnCountProperty->IsValueString() &&
                            columnCountProperty->GetValueString().CmpNoCase(L"all") == 0)
                            {
                            currentCell->SetColumnCount(table->GetColumnCount());
                            }
                        else if (columnCountProperty->IsValueNumber())
                            {
                            currentCell->SetColumnCount(columnCountProperty->GetValueNumber());
                            }
                        }
                    // row count
                    const auto rowCountProperty =
                        cellUpdate->GetProperty(L"row-count");
                    if (rowCountProperty->IsOk())
                        {
                        if (rowCountProperty->IsValueString() &&
                            rowCountProperty->GetValueString().CmpNoCase(L"all") == 0)
                            {
                            currentCell->SetRowCount(table->GetRowCount());
                            }
                        else if (rowCountProperty->IsValueNumber())
                            {
                            currentCell->SetRowCount(rowCountProperty->GetValueNumber());
                            }
                        }
                    // value
                    const auto valueProperty = cellUpdate->GetProperty(L"value");
                    if (valueProperty->IsOk())
                        {
                        if (valueProperty->IsValueString())
                            {
                            currentCell->SetValue(valueProperty->GetValueString());
                            }
                        else if (valueProperty->IsValueNumber())
                            { currentCell->SetValue(valueProperty->GetValueNumber()); }
                        else if (valueProperty->IsValueNull())
                            { currentCell->SetValue(wxEmptyString); }
                        }
                    // background color
                    const wxColour bgcolor(
                        ConvertColor(cellUpdate->GetProperty(L"background")->GetValueString()));
                    if (bgcolor.IsOk())
                        { currentCell->SetBackgroundColor(bgcolor); }

                    // prefix
                    if (cellUpdate->GetProperty(L"prefix")->IsOk())
                        {
                        currentCell->SetPrefix(
                            cellUpdate->GetProperty(L"prefix")->GetValueString());
                        }

                    // is it highlighted
                    if (cellUpdate->GetProperty(L"highlight")->IsOk())
                        {
                        currentCell->Highlight(
                            cellUpdate->GetProperty(L"highlight")->GetValueBool());
                        }

                    // font attributes
                    if (cellUpdate->GetProperty(L"bold")->IsOk())
                        {
                        cellUpdate->GetProperty(L"bold")->GetValueBool() ?
                            currentCell->GetFont().MakeBold() :
                            currentCell->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                        }

                    // outer border toggles
                    const auto outerBorderToggles =
                        cellUpdate->GetProperty(L"show-borders")->GetValueArrayBool();
                    if (outerBorderToggles.size() >= 1)
                        { currentCell->ShowTopBorder(outerBorderToggles[0]); }
                    if (outerBorderToggles.size() >= 2)
                        { currentCell->ShowRightBorder(outerBorderToggles[1]); }
                    if (outerBorderToggles.size() >= 3)
                        { currentCell->ShowBottomBorder(outerBorderToggles[2]); }
                    if (outerBorderToggles.size() >= 4)
                        { currentCell->ShowLeftBorder(outerBorderToggles[3]); }

                    const auto textAlignment = ConvertTextAlignment(
                        cellUpdate->GetProperty(L"text-alignment")->GetValueString());
                    if (textAlignment.has_value())
                        { currentCell->SetTextAlignment(textAlignment.value()); }

                    // horizontal page alignment
                    const auto hPageAlignment =
                        cellUpdate->GetProperty(L"horizontal-page-alignment")->GetValueString();
                    if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                        { currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned); }
                    else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                        { currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned); }
                    else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                        { currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered); }
                    }
                }
            }

        const auto annotationsNode = graphNode->GetProperty(L"cell-annotations")->GetValueArrayObject();
        if (annotationsNode.size())
            {
            for (const auto& annotation : annotationsNode)
                {
                Table::CellAnnotation cellAnnotation{ annotation->GetProperty(L"value")->GetValueString() };
                if (annotation->GetProperty(L"side")->IsOk())
                    {
                    cellAnnotation.m_side =
                        (annotation->GetProperty(L"side")->GetValueString().CmpNoCase(L"left") == 0) ?
                        Side::Left : Side::Right;
                    }
                cellAnnotation.m_connectionLinePen = table->GetHighlightPen();
                LoadPen(annotation->GetProperty(L"pen"), cellAnnotation.m_connectionLinePen.value());
                cellAnnotation.m_bgColor = ConvertColor(annotation->GetProperty(L"background")->GetValueString());
                
                const auto cellsNode = annotation->GetProperty(L"cells");
                if (cellsNode->IsOk() && cellsNode->IsValueObject())
                    {
                    const auto outliersNode = cellsNode->GetProperty(L"column-outliers");
                    const auto topNNode = cellsNode->GetProperty(L"column-top-n");
                    if (outliersNode->IsOk() && outliersNode->IsValueString())
                        {
                        const auto colIndex = table->FindColumnIndex(outliersNode->GetValueString());
                        if (colIndex.has_value())
                            {
                            cellAnnotation.m_cells = table->GetOutliers(colIndex.value());
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    else if (topNNode->IsOk() && topNNode->IsValueString())
                        {
                        const auto colIndex = table->FindColumnIndex(topNNode->GetValueString());
                        if (colIndex.has_value())
                            {
                            cellAnnotation.m_cells = table->GetTopN(colIndex.value(),
                                                                    cellsNode->GetProperty(L"n")->GetValueNumber(1));
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    }
                }
            }

        // assign footnotes after all cells have been updated
        const auto footnotesNode = graphNode->GetProperty(L"footnotes")->GetValueArrayObject();
        if (footnotesNode.size())
            {
            for (const auto& ftNode : footnotesNode)
                {
                table->AddFootnote(
                    ExpandConstants(ftNode->GetProperty(L"value")->GetValueString()),
                    ExpandConstants(ftNode->GetProperty(L"footnote")->GetValueString()));
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, table);
        return table;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::ExpandNumericConstant(wxString str) const
        {
        if (str.starts_with(L"{{") && str.ends_with(L"}}"))
            { str = str.substr(2, str.length() - 4); }
        const auto foundVal = m_values.find(str);
        if (foundVal != m_values.cend())
            {
            if (const auto dVal{ std::get_if<double>(&foundVal->second) };
                dVal != nullptr && !std::isnan(*dVal))
                { return *dVal; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ExpandConstants(wxString str) const
        {
        const wxRegEx re(L"{{([^}]+)}}");
        size_t start{ 0 }, len{ 0 };
        std::wstring_view processText(str.wc_str());
        std::map<wxString, wxString> replacements;
        while (re.Matches(processText.data()))
            {
            // catalog all the placeholders and their replacements
            re.GetMatch(&start, &len, 0);
            const auto foundVal = m_values.find(
                re.GetMatch(processText.data(), 1));
            if (foundVal != m_values.cend())
                {
                if (const auto strVal{ std::get_if<wxString>(&foundVal->second) };
                    strVal != nullptr)
                    {
                    replacements.insert_or_assign(
                        std::wstring(processText.substr(start, len)), *strVal);
                    }
                else if (const auto dVal{ std::get_if<double>(&foundVal->second) };
                         dVal != nullptr)
                    {
                    if (std::isnan(*dVal))
                        {
                        replacements.insert_or_assign(
                            std::wstring(processText.substr(start, len)), wxEmptyString);
                        }
                    else
                        {
                        replacements.insert_or_assign(
                            std::wstring(processText.substr(start, len)),
                            wxNumberFormatter::ToString(*dVal, 2,
                                wxNumberFormatter::Style::Style_WithThousandsSep|
                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
                        }
                    }
                }
            processText = processText.substr(start + len);
            }

        // now, replace the placeholders with the user-defined values mapped to them
        for (const auto& rep : replacements)
            { str.Replace(rep.first, rep.second); }

        return str;
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(wxString colorStr)
        {
        // NOTE: do not edit this. This is generated from the file "tools/Build Color List.R"
        // Refer to the instructions in that file for updating this map.
        static const std::map<std::wstring_view, Colors::Color> values =
            {
            { L"afternoon", Colors::Color::Afternoon },
            { L"airforceblue", Colors::Color::AirForceBlue },
            { L"alexandrite", Colors::Color::Alexandrite },
            { L"aliceblue", Colors::Color::AliceBlue },
            { L"almond", Colors::Color::Almond },
            { L"amaranth", Colors::Color::Amaranth },
            { L"amber", Colors::Color::Amber },
            { L"amberwave", Colors::Color::AmberWave },
            { L"amethyst", Colors::Color::Amethyst },
            { L"androidgreen", Colors::Color::AndroidGreen },
            { L"antiquebrass", Colors::Color::AntiqueBrass },
            { L"antiquefuchsia", Colors::Color::AntiqueFuchsia },
            { L"antiquewhite", Colors::Color::AntiqueWhite },
            { L"ao", Colors::Color::Ao },
            { L"appleblossom", Colors::Color::Appleblossom },
            { L"applegreen", Colors::Color::AppleGreen },
            { L"apricot", Colors::Color::Apricot },
            { L"aqua", Colors::Color::Aqua },
            { L"aquamarine", Colors::Color::Aquamarine },
            { L"aquitaine", Colors::Color::Aquitaine },
            { L"armygreen", Colors::Color::ArmyGreen },
            { L"ashgrey", Colors::Color::AshGrey },
            { L"asparagus", Colors::Color::Asparagus },
            { L"auburn", Colors::Color::Auburn },
            { L"aureolin", Colors::Color::Aureolin },
            { L"avocado", Colors::Color::Avocado },
            { L"azure", Colors::Color::Azure },
            { L"azuremist", Colors::Color::AzureMist },
            { L"babyblue", Colors::Color::BabyBlue },
            { L"babypink", Colors::Color::BabyPink },
            { L"bananayellow", Colors::Color::BananaYellow },
            { L"bark", Colors::Color::Bark },
            { L"basketbeige", Colors::Color::BasketBeige },
            { L"battleshipgrey", Colors::Color::BattleshipGrey },
            { L"bazaar", Colors::Color::Bazaar },
            { L"beaublue", Colors::Color::BeauBlue },
            { L"beige", Colors::Color::Beige },
            { L"belvederecream", Colors::Color::BelvedereCream },
            { L"bistre", Colors::Color::Bistre },
            { L"black", Colors::Color::Black },
            { L"blanchedalmond", Colors::Color::BlanchedAlmond },
            { L"blizzardblue", Colors::Color::BlizzardBlue },
            { L"blond", Colors::Color::Blond },
            { L"blue", Colors::Color::Blue },
            { L"bluebell", Colors::Color::BlueBell },
            { L"blueberry", Colors::Color::Blueberry },
            { L"bluesky", Colors::Color::BlueSky },
            { L"blush", Colors::Color::Blush },
            { L"bondiblue", Colors::Color::BondiBlue },
            { L"boysenberry", Colors::Color::Boysenberry },
            { L"brass", Colors::Color::Brass },
            { L"brickred", Colors::Color::BrickRed },
            { L"britishracinggreen", Colors::Color::BritishRacingGreen },
            { L"bronze", Colors::Color::Bronze },
            { L"brown", Colors::Color::Brown },
            { L"brownstone", Colors::Color::Brownstone },
            { L"bubblegum", Colors::Color::BubbleGum },
            { L"bubbles", Colors::Color::Bubbles },
            { L"bungalowbeige", Colors::Color::BungalowBeige },
            { L"burgundy", Colors::Color::Burgundy },
            { L"burlywood", Colors::Color::Burlywood },
            { L"burntsienna", Colors::Color::BurntSienna },
            { L"burntumber", Colors::Color::BurntUmber },
            { L"byzantium", Colors::Color::Byzantium },
            { L"cadet", Colors::Color::Cadet },
            { L"cadetblue", Colors::Color::CadetBlue },
            { L"cadetgrey", Colors::Color::CadetGrey },
            { L"cafeaulait", Colors::Color::CafeAuLait },
            { L"cafenoir", Colors::Color::CafeNoir },
            { L"camouflagegreen", Colors::Color::CamouflageGreen },
            { L"canary", Colors::Color::Canary },
            { L"candyapple", Colors::Color::CandyApple },
            { L"candypink", Colors::Color::CandyPink },
            { L"capri", Colors::Color::Capri },
            { L"caputmortuum", Colors::Color::CaputMortuum },
            { L"caramel", Colors::Color::Caramel },
            { L"cardinal", Colors::Color::Cardinal },
            { L"caribbeangreen", Colors::Color::CaribbeanGreen },
            { L"carmine", Colors::Color::Carmine },
            { L"carnationpink", Colors::Color::CarnationPink },
            { L"carnelian", Colors::Color::Carnelian },
            { L"carrotorange", Colors::Color::CarrotOrange },
            { L"cascadegreen", Colors::Color::CascadeGreen },
            { L"cayenne", Colors::Color::Cayenne },
            { L"celadon", Colors::Color::Celadon },
            { L"celeste", Colors::Color::Celeste },
            { L"celestialblue", Colors::Color::CelestialBlue },
            { L"ceramic", Colors::Color::Ceramic },
            { L"cerise", Colors::Color::Cerise },
            { L"cerulean", Colors::Color::Cerulean },
            { L"chamoisee", Colors::Color::Chamoisee },
            { L"champagne", Colors::Color::Champagne },
            { L"charcoal", Colors::Color::Charcoal },
            { L"cherry", Colors::Color::Cherry },
            { L"cherryblossompink", Colors::Color::CherryBlossomPink },
            { L"chestnut", Colors::Color::Chestnut },
            { L"chinesered", Colors::Color::ChineseRed },
            { L"chocolate", Colors::Color::Chocolate },
            { L"cinereous", Colors::Color::Cinereous },
            { L"cinnamon", Colors::Color::Cinnamon },
            { L"citrine", Colors::Color::Citrine },
            { L"classicfrenchgray", Colors::Color::ClassicFrenchGray },
            { L"classicrose", Colors::Color::ClassicRose },
            { L"cobalt", Colors::Color::Cobalt },
            { L"coffee", Colors::Color::Coffee },
            { L"cooledblue", Colors::Color::CooledBlue },
            { L"coolgrey", Colors::Color::CoolGrey },
            { L"copper", Colors::Color::Copper },
            { L"copperrose", Colors::Color::CopperRose },
            { L"coral", Colors::Color::Coral },
            { L"coralpink", Colors::Color::CoralPink },
            { L"cordovan", Colors::Color::Cordovan },
            { L"corn", Colors::Color::Corn },
            { L"cornflower", Colors::Color::Cornflower },
            { L"cornflowerblue", Colors::Color::CornflowerBlue },
            { L"cottoncandy", Colors::Color::CottonCandy },
            { L"cream", Colors::Color::Cream },
            { L"crimson", Colors::Color::Crimson },
            { L"cyan", Colors::Color::Cyan },
            { L"daffodil", Colors::Color::Daffodil },
            { L"daisy", Colors::Color::Daisy },
            { L"dandelion", Colors::Color::Dandelion },
            { L"darkblue", Colors::Color::DarkBlue },
            { L"darkbrown", Colors::Color::DarkBrown },
            { L"darkgray", Colors::Color::DarkGray },
            { L"darkgreen", Colors::Color::DarkGreen },
            { L"davygrey", Colors::Color::DavyGrey },
            { L"denim", Colors::Color::Denim },
            { L"desert", Colors::Color::Desert },
            { L"desertsand", Colors::Color::DesertSand },
            { L"dimgray", Colors::Color::DimGray },
            { L"dodgerblue", Colors::Color::DodgerBlue },
            { L"dollarbill", Colors::Color::DollarBill },
            { L"doverwhite", Colors::Color::DoverWhite },
            { L"drab", Colors::Color::Drab },
            { L"dressyrose", Colors::Color::DressyRose },
            { L"earthyellow", Colors::Color::EarthYellow },
            { L"edgygold", Colors::Color::EdgyGold },
            { L"eggplant", Colors::Color::Eggplant },
            { L"eggshell", Colors::Color::Eggshell },
            { L"egyptianblue", Colors::Color::EgyptianBlue },
            { L"electricblue", Colors::Color::ElectricBlue },
            { L"emerald", Colors::Color::Emerald },
            { L"evergreenfog", Colors::Color::EvergreenFog },
            { L"exuberantpink", Colors::Color::ExuberantPink },
            { L"fallow", Colors::Color::Fallow },
            { L"falured", Colors::Color::FaluRed },
            { L"famous", Colors::Color::Famous },
            { L"favoritejeans", Colors::Color::FavoriteJeans },
            { L"fawn", Colors::Color::Fawn },
            { L"feldgrau", Colors::Color::Feldgrau },
            { L"fern", Colors::Color::Fern },
            { L"ferngreen", Colors::Color::FernGreen },
            { L"fielddrab", Colors::Color::FieldDrab },
            { L"firebrick", Colors::Color::Firebrick },
            { L"fireenginered", Colors::Color::FireEngineRed },
            { L"fireweed", Colors::Color::Fireweed },
            { L"firework", Colors::Color::Firework },
            { L"flamingopink", Colors::Color::FlamingoPink },
            { L"flatteringpeach", Colors::Color::FlatteringPeach },
            { L"flax", Colors::Color::Flax },
            { L"fluorescentorange", Colors::Color::FluorescentOrange },
            { L"fluorescentpink", Colors::Color::FluorescentPink },
            { L"folksygold", Colors::Color::FolksyGold },
            { L"forestgreen", Colors::Color::ForestGreen },
            { L"forgetmenot", Colors::Color::ForgetMeNot },
            { L"frenchbeige", Colors::Color::FrenchBeige },
            { L"frenchblue", Colors::Color::FrenchBlue },
            { L"frenchlilac", Colors::Color::FrenchLilac },
            { L"frenchrose", Colors::Color::FrenchRose },
            { L"frolic", Colors::Color::Frolic },
            { L"frosting", Colors::Color::Frosting },
            { L"frostwork", Colors::Color::Frostwork },
            { L"fuchsia", Colors::Color::Fuchsia },
            { L"fulvous", Colors::Color::Fulvous },
            { L"fuzzywuzzy", Colors::Color::FuzzyWuzzy },
            { L"gamboge", Colors::Color::Gamboge },
            { L"ghostwhite", Colors::Color::GhostWhite },
            { L"ginger", Colors::Color::Ginger },
            { L"glacierblue", Colors::Color::GlacierBlue },
            { L"glaucous", Colors::Color::Glaucous },
            { L"gold", Colors::Color::Gold },
            { L"goldenbrown", Colors::Color::GoldenBrown },
            { L"goldenrod", Colors::Color::Goldenrod },
            { L"goldenyellow", Colors::Color::GoldenYellow },
            { L"goldleaf", Colors::Color::GoldLeaf },
            { L"grannysmithapple", Colors::Color::GrannySmithApple },
            { L"grass", Colors::Color::Grass },
            { L"gray", Colors::Color::Gray },
            { L"grayasparagus", Colors::Color::GrayAsparagus },
            { L"green", Colors::Color::Green },
            { L"grullo", Colors::Color::Grullo },
            { L"halayaube", Colors::Color::HalayaUbe },
            { L"harlequin", Colors::Color::Harlequin },
            { L"harvestgold", Colors::Color::HarvestGold },
            { L"heartgold", Colors::Color::HeartGold },
            { L"heliotrope", Colors::Color::Heliotrope },
            { L"hickorysmoke", Colors::Color::HickorySmoke },
            { L"honeydew", Colors::Color::Honeydew },
            { L"hookergreen", Colors::Color::HookerGreen },
            { L"hotmagenta", Colors::Color::HotMagenta },
            { L"hotpink", Colors::Color::HotPink },
            { L"huntergreen", Colors::Color::HunterGreen },
            { L"ice", Colors::Color::Ice },
            { L"icterine", Colors::Color::Icterine },
            { L"inchworm", Colors::Color::Inchworm },
            { L"indigo", Colors::Color::Indigo },
            { L"iris", Colors::Color::Iris },
            { L"ivory", Colors::Color::Ivory },
            { L"jade", Colors::Color::Jade },
            { L"jasmine", Colors::Color::Jasmine },
            { L"jasper", Colors::Color::Jasper },
            { L"jazzagecoral", Colors::Color::JazzAgeCoral },
            { L"jazzberryjam", Colors::Color::JazzberryJam },
            { L"jonquil", Colors::Color::Jonquil },
            { L"junglegreen", Colors::Color::JungleGreen },
            { L"jutebrown", Colors::Color::JuteBrown },
            { L"kellygreen", Colors::Color::KellyGreen },
            { L"khaki", Colors::Color::Khaki },
            { L"latte", Colors::Color::Latte },
            { L"laurelgreen", Colors::Color::LaurelGreen },
            { L"lavender", Colors::Color::Lavender },
            { L"leaves", Colors::Color::Leaves },
            { L"lemon", Colors::Color::Lemon },
            { L"lemonchiffon", Colors::Color::LemonChiffon },
            { L"lemonlime", Colors::Color::LemonLime },
            { L"lemonyellow", Colors::Color::LemonYellow },
            { L"lightapricot", Colors::Color::LightApricot },
            { L"lightblue", Colors::Color::LightBlue },
            { L"lightbrown", Colors::Color::LightBrown },
            { L"lightcarminepink", Colors::Color::LightCarminePink },
            { L"lightcoral", Colors::Color::LightCoral },
            { L"lightcornflowerblue", Colors::Color::LightCornflowerBlue },
            { L"lightgray", Colors::Color::LightGray },
            { L"lightseafoam", Colors::Color::LightSeafoam },
            { L"lilac", Colors::Color::Lilac },
            { L"lime", Colors::Color::Lime },
            { L"limegreen", Colors::Color::LimeGreen },
            { L"lincolngreen", Colors::Color::LincolnGreen },
            { L"linen", Colors::Color::Linen },
            { L"lion", Colors::Color::Lion },
            { L"lust", Colors::Color::Lust },
            { L"macaroniandcheese", Colors::Color::MacaroniAndCheese },
            { L"magenta", Colors::Color::Magenta },
            { L"magicmint", Colors::Color::MagicMint },
            { L"magnolia", Colors::Color::Magnolia },
            { L"mahogany", Colors::Color::Mahogany },
            { L"maize", Colors::Color::Maize },
            { L"majorelleblue", Colors::Color::MajorelleBlue },
            { L"malachite", Colors::Color::Malachite },
            { L"manatee", Colors::Color::Manatee },
            { L"mangotango", Colors::Color::MangoTango },
            { L"marble", Colors::Color::Marble },
            { L"maroon", Colors::Color::Maroon },
            { L"mauve", Colors::Color::Mauve },
            { L"mauvelous", Colors::Color::Mauvelous },
            { L"mauvetaupe", Colors::Color::MauveTaupe },
            { L"meadow", Colors::Color::Meadow },
            { L"melon", Colors::Color::Melon },
            { L"metal", Colors::Color::Metal },
            { L"mint", Colors::Color::Mint },
            { L"mintcream", Colors::Color::MintCream },
            { L"mintgreen", Colors::Color::MintGreen },
            { L"moccasin", Colors::Color::Moccasin },
            { L"modebeige", Colors::Color::ModeBeige },
            { L"moonstoneblue", Colors::Color::MoonstoneBlue },
            { L"moss", Colors::Color::Moss },
            { L"mossgreen", Colors::Color::MossGreen },
            { L"mountainmeadow", Colors::Color::MountainMeadow },
            { L"mountbattenpink", Colors::Color::MountbattenPink },
            { L"mulberry", Colors::Color::Mulberry },
            { L"munsell", Colors::Color::Munsell },
            { L"mustard", Colors::Color::Mustard },
            { L"myrtle", Colors::Color::Myrtle },
            { L"nadeshikopink", Colors::Color::NadeshikoPink },
            { L"napiergreen", Colors::Color::NapierGreen },
            { L"naplesyellow", Colors::Color::NaplesYellow },
            { L"navajowhite", Colors::Color::NavajoWhite },
            { L"navel", Colors::Color::Navel },
            { L"navy", Colors::Color::Navy },
            { L"navyblue", Colors::Color::NavyBlue },
            { L"neoncarrot", Colors::Color::NeonCarrot },
            { L"neonfuchsia", Colors::Color::NeonFuchsia },
            { L"neongreen", Colors::Color::NeonGreen },
            { L"newsprint", Colors::Color::Newsprint },
            { L"ocean", Colors::Color::Ocean },
            { L"oceanboatblue", Colors::Color::OceanBoatBlue },
            { L"oceanic", Colors::Color::Oceanic },
            { L"ochre", Colors::Color::Ochre },
            { L"octobermist", Colors::Color::OctoberMist },
            { L"officegreen", Colors::Color::OfficeGreen },
            { L"oldgold", Colors::Color::OldGold },
            { L"oldlace", Colors::Color::OldLace },
            { L"olive", Colors::Color::Olive },
            { L"olivedrab", Colors::Color::OliveDrab },
            { L"olivegreen", Colors::Color::OliveGreen },
            { L"olivine", Colors::Color::Olivine },
            { L"onyx", Colors::Color::Onyx },
            { L"operamauve", Colors::Color::OperaMauve },
            { L"orange", Colors::Color::Orange },
            { L"orangered", Colors::Color::OrangeRed },
            { L"orangeyellow", Colors::Color::OrangeYellow },
            { L"orchid", Colors::Color::Orchid },
            { L"origamiwhite", Colors::Color::OrigamiWhite },
            { L"otterbrown", Colors::Color::OtterBrown },
            { L"outerspace", Colors::Color::OuterSpace },
            { L"outrageousorange", Colors::Color::OutrageousOrange },
            { L"overcast", Colors::Color::Overcast },
            { L"oxfordblue", Colors::Color::OxfordBlue },
            { L"pacificblue", Colors::Color::PacificBlue },
            { L"pansypurple", Colors::Color::PansyPurple },
            { L"paper", Colors::Color::Paper },
            { L"parisgreen", Colors::Color::ParisGreen },
            { L"pastelblue", Colors::Color::PastelBlue },
            { L"pastelbrown", Colors::Color::PastelBrown },
            { L"pastelgray", Colors::Color::PastelGray },
            { L"pastelgreen", Colors::Color::PastelGreen },
            { L"pastelmagenta", Colors::Color::PastelMagenta },
            { L"pastelorange", Colors::Color::PastelOrange },
            { L"pastelpink", Colors::Color::PastelPink },
            { L"pastelpurple", Colors::Color::PastelPurple },
            { L"pastelred", Colors::Color::PastelRed },
            { L"pastelviolet", Colors::Color::PastelViolet },
            { L"pastelyellow", Colors::Color::PastelYellow },
            { L"patriarch", Colors::Color::Patriarch },
            { L"paynegrey", Colors::Color::PayneGrey },
            { L"peach", Colors::Color::Peach },
            { L"peacockblue", Colors::Color::PeacockBlue },
            { L"pear", Colors::Color::Pear },
            { L"pearl", Colors::Color::Pearl },
            { L"pencilyellow", Colors::Color::PencilYellow },
            { L"peridot", Colors::Color::Peridot },
            { L"periwinkle", Colors::Color::Periwinkle },
            { L"petal", Colors::Color::Petal },
            { L"pewter", Colors::Color::Pewter },
            { L"phlox", Colors::Color::Phlox },
            { L"phthaloblue", Colors::Color::PhthaloBlue },
            { L"phthalogreen", Colors::Color::PhthaloGreen },
            { L"piggypink", Colors::Color::PiggyPink },
            { L"pinegreen", Colors::Color::PineGreen },
            { L"pink", Colors::Color::Pink },
            { L"pinkeraser", Colors::Color::PinkEraser },
            { L"pinkflamingo", Colors::Color::PinkFlamingo },
            { L"pinkpearl", Colors::Color::PinkPearl },
            { L"pinkshadow", Colors::Color::PinkShadow },
            { L"pinksherbet", Colors::Color::PinkSherbet },
            { L"pinktulip", Colors::Color::PinkTulip },
            { L"pinkybeige", Colors::Color::PinkyBeige },
            { L"pistachio", Colors::Color::Pistachio },
            { L"platinum", Colors::Color::Platinum },
            { L"plum", Colors::Color::Plum },
            { L"poppy", Colors::Color::Poppy },
            { L"powderblue", Colors::Color::PowderBlue },
            { L"practicalbeige", Colors::Color::PracticalBeige },
            { L"prussianblue", Colors::Color::PrussianBlue },
            { L"psychedelicpurple", Colors::Color::PsychedelicPurple },
            { L"puce", Colors::Color::Puce },
            { L"pumpkin", Colors::Color::Pumpkin },
            { L"purewhite", Colors::Color::PureWhite },
            { L"purple", Colors::Color::Purple },
            { L"purpleheart", Colors::Color::PurpleHeart },
            { L"purplemountainmajesty", Colors::Color::PurpleMountainMajesty },
            { L"purplepizzazz", Colors::Color::PurplePizzazz },
            { L"purpletaupe", Colors::Color::PurpleTaupe },
            { L"rackley", Colors::Color::Rackley },
            { L"radiantlilac", Colors::Color::RadiantLilac },
            { L"rain", Colors::Color::Rain },
            { L"raspberry", Colors::Color::Raspberry },
            { L"raspberryglace", Colors::Color::RaspberryGlace },
            { L"raspberrypink", Colors::Color::RaspberryPink },
            { L"razzledazzlerose", Colors::Color::RazzleDazzleRose },
            { L"razzmatazz", Colors::Color::Razzmatazz },
            { L"red", Colors::Color::Red },
            { L"redtomato", Colors::Color::RedTomato },
            { L"robineggblue", Colors::Color::RobinEggBlue },
            { L"rose", Colors::Color::Rose },
            { L"rosegold", Colors::Color::RoseGold },
            { L"rosemadder", Colors::Color::RoseMadder },
            { L"rosepink", Colors::Color::RosePink },
            { L"rosequartz", Colors::Color::RoseQuartz },
            { L"rosetan", Colors::Color::RoseTan },
            { L"rosetaupe", Colors::Color::RoseTaupe },
            { L"rosevale", Colors::Color::RoseVale },
            { L"rosewood", Colors::Color::Rosewood },
            { L"rossocorsa", Colors::Color::RossoCorsa },
            { L"rosybrown", Colors::Color::RosyBrown },
            { L"rosyoutlook", Colors::Color::RosyOutlook },
            { L"royalblue", Colors::Color::RoyalBlue },
            { L"royalfuchsia", Colors::Color::RoyalFuchsia },
            { L"royalpurple", Colors::Color::RoyalPurple },
            { L"ruby", Colors::Color::Ruby },
            { L"ruddybrown", Colors::Color::RuddyBrown },
            { L"rust", Colors::Color::Rust },
            { L"saddlebrown", Colors::Color::SaddleBrown },
            { L"safetyorange", Colors::Color::SafetyOrange },
            { L"saffron", Colors::Color::Saffron },
            { L"salmon", Colors::Color::Salmon },
            { L"salmonpink", Colors::Color::SalmonPink },
            { L"salonrose", Colors::Color::SalonRose },
            { L"sand", Colors::Color::Sand },
            { L"sanddune", Colors::Color::SandDune },
            { L"sandstorm", Colors::Color::Sandstorm },
            { L"sandybrown", Colors::Color::SandyBrown },
            { L"sapgreen", Colors::Color::SapGreen },
            { L"sapphire", Colors::Color::Sapphire },
            { L"satinsheengold", Colors::Color::SatinSheenGold },
            { L"scarlet", Colors::Color::Scarlet },
            { L"schoolbusyellow", Colors::Color::SchoolBusYellow },
            { L"seablue", Colors::Color::SeaBlue },
            { L"seagreen", Colors::Color::SeaGreen },
            { L"sealbrown", Colors::Color::SealBrown },
            { L"seashell", Colors::Color::Seashell },
            { L"sepia", Colors::Color::Sepia },
            { L"serenity", Colors::Color::Serenity },
            { L"shadow", Colors::Color::Shadow },
            { L"shamrock", Colors::Color::Shamrock },
            { L"sienna", Colors::Color::Sienna },
            { L"silver", Colors::Color::Silver },
            { L"sinopia", Colors::Color::Sinopia },
            { L"skobeloff", Colors::Color::Skobeloff },
            { L"sky", Colors::Color::Sky },
            { L"skyblue", Colors::Color::SkyBlue },
            { L"slate", Colors::Color::Slate },
            { L"slateblue", Colors::Color::SlateBlue },
            { L"slategray", Colors::Color::SlateGray },
            { L"slytherin1", Colors::Color::Slytherin1 },
            { L"slytherin2", Colors::Color::Slytherin2 },
            { L"slytherin3", Colors::Color::Slytherin3 },
            { L"slytherin4", Colors::Color::Slytherin4 },
            { L"smalt", Colors::Color::Smalt },
            { L"smokeytopaz", Colors::Color::SmokeyTopaz },
            { L"smokyblack", Colors::Color::SmokyBlack },
            { L"smokysalmon", Colors::Color::SmokySalmon },
            { L"snow", Colors::Color::Snow },
            { L"spicedcider", Colors::Color::SpicedCider },
            { L"spirodiscoball", Colors::Color::SpiroDiscoBall },
            { L"springbud", Colors::Color::SpringBud },
            { L"springgreen", Colors::Color::SpringGreen },
            { L"steel", Colors::Color::Steel },
            { L"steelblue", Colors::Color::SteelBlue },
            { L"stem", Colors::Color::Stem },
            { L"stizza", Colors::Color::Stizza },
            { L"stormcloud", Colors::Color::Stormcloud },
            { L"straw", Colors::Color::Straw },
            { L"strawberry", Colors::Color::Strawberry },
            { L"studiomauve", Colors::Color::StudioMauve },
            { L"sunbeamyellow", Colors::Color::SunbeamYellow },
            { L"sunflower", Colors::Color::Sunflower },
            { L"sunglow", Colors::Color::Sunglow },
            { L"sunset", Colors::Color::Sunset },
            { L"sunsetorange", Colors::Color::SunsetOrange },
            { L"sveltesage", Colors::Color::SvelteSage },
            { L"tan", Colors::Color::Tan },
            { L"tangelo", Colors::Color::Tangelo },
            { L"tangerine", Colors::Color::Tangerine },
            { L"taupe", Colors::Color::Taupe },
            { L"tawny", Colors::Color::Tawny },
            { L"teagreen", Colors::Color::TeaGreen },
            { L"teal", Colors::Color::Teal },
            { L"tearose", Colors::Color::TeaRose },
            { L"terracotta", Colors::Color::TerraCotta },
            { L"thistle", Colors::Color::Thistle },
            { L"thulianpink", Colors::Color::ThulianPink },
            { L"thundercloud", Colors::Color::ThunderCloud },
            { L"tiffanyblue", Colors::Color::TiffanyBlue },
            { L"tigereye", Colors::Color::TigerEye },
            { L"timberwolf", Colors::Color::Timberwolf },
            { L"titaniumyellow", Colors::Color::TitaniumYellow },
            { L"toffee", Colors::Color::Toffee },
            { L"tomato", Colors::Color::Tomato },
            { L"toolbox", Colors::Color::Toolbox },
            { L"topaz", Colors::Color::Topaz },
            { L"tractorred", Colors::Color::TractorRed },
            { L"tradewind", Colors::Color::Tradewind },
            { L"tricornblack", Colors::Color::TricornBlack },
            { L"trolleygrey", Colors::Color::TrolleyGrey },
            { L"tropicalrainforest", Colors::Color::TropicalRainForest },
            { L"tumbleweed", Colors::Color::Tumbleweed },
            { L"turkishrose", Colors::Color::TurkishRose },
            { L"turquoise", Colors::Color::Turquoise },
            { L"twilightlavender", Colors::Color::TwilightLavender },
            { L"tyrianpurple", Colors::Color::TyrianPurple },
            { L"umber", Colors::Color::Umber },
            { L"unmellowyellow", Colors::Color::UnmellowYellow },
            { L"urbanputty", Colors::Color::UrbanPutty },
            { L"urobilin", Colors::Color::Urobilin },
            { L"vanilla", Colors::Color::Vanilla },
            { L"vegasgold", Colors::Color::VegasGold },
            { L"venetianred", Colors::Color::VenetianRed },
            { L"verdigris", Colors::Color::Verdigris },
            { L"vermilion", Colors::Color::Vermilion },
            { L"veronica", Colors::Color::Veronica },
            { L"violet", Colors::Color::Violet },
            { L"viridian", Colors::Color::Viridian },
            { L"vividauburn", Colors::Color::VividAuburn },
            { L"vividburgundy", Colors::Color::VividBurgundy },
            { L"vividcerise", Colors::Color::VividCerise },
            { L"vividtangerine", Colors::Color::VividTangerine },
            { L"vividviolet", Colors::Color::VividViolet },
            { L"voguegreen", Colors::Color::VogueGreen },
            { L"warmblack", Colors::Color::WarmBlack },
            { L"warmgray", Colors::Color::WarmGray },
            { L"waterfall", Colors::Color::Waterfall },
            { L"waterspout", Colors::Color::Waterspout },
            { L"watery", Colors::Color::Watery },
            { L"wave", Colors::Color::Wave },
            { L"wenge", Colors::Color::Wenge },
            { L"wheat", Colors::Color::Wheat },
            { L"white", Colors::Color::White },
            { L"wholewheat", Colors::Color::WholeWheat },
            { L"wildblueyonder", Colors::Color::WildBlueYonder },
            { L"wildstrawberry", Colors::Color::WildStrawberry },
            { L"wildwatermelon", Colors::Color::WildWatermelon },
            { L"wine", Colors::Color::Wine },
            { L"wisteria", Colors::Color::Wisteria },
            { L"wood", Colors::Color::Wood },
            { L"xanadu", Colors::Color::Xanadu },
            { L"yellow", Colors::Color::Yellow },
            { L"yellowpepper", Colors::Color::YellowPepper },
            { L"zinnwalditebrown", Colors::Color::ZinnwalditeBrown }
            };

        // in case the color is a user-defined constant in the file
        colorStr = ExpandConstants(colorStr);

        // see if it is one of our defined colors
        auto foundPos = values.find(std::wstring_view(colorStr.MakeLower().wc_str()));
        if (foundPos != values.cend())
            { return Colors::ColorBrewer::GetColor(foundPos->second); }

        // otherwise, load it with wx (which will support hex coded values and other names)
        return wxColour(colorStr);
        }

    //---------------------------------------------------
    std::optional<size_t> ReportBuilder::LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
        const size_t columnCount, const size_t columnRow, std::shared_ptr<Graphs::Table> table)
        {
        std::optional<size_t> position;
        const auto origin = positionNode->GetProperty(L"origin");
        if (origin->IsOk())
            {
            if (origin->IsValueString())
                {
                const auto originStr{ origin->GetValueString() };
                if (originStr.CmpNoCase(L"last-column") == 0)
                    { position = columnCount-1; }
                else if (originStr.CmpNoCase(L"last-row") == 0)
                    { position = columnRow-1; }
                else if (const auto colPos = (table ? table->FindColumnIndex(originStr) : std::nullopt);
                         colPos.has_value())
                    { return colPos; }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown table position origin value."), originStr).ToUTF8());
                    }
                }
            else if (origin->IsValueNumber())
                { position = origin->GetValueNumber(); }
            }
        std::optional<double> doubleStartOffset =
            positionNode->GetProperty(L"offset")->IsOk() ?
            std::optional<double>(positionNode->GetProperty(L"offset")->GetValueNumber()) :
            std::nullopt;
        if (position.has_value() && doubleStartOffset.has_value())
            { position.value() += doubleStartOffset.value(); }

        return position;
        }

    //---------------------------------------------------
    std::shared_ptr<Brushes::Schemes::BrushScheme> ReportBuilder::LoadBrushScheme(
        const wxSimpleJSON::Ptr_t& brushSchemeNode)
        {
        const auto brushStylesNode = brushSchemeNode->GetProperty(L"brush-styles");
        if (brushStylesNode->IsOk() && brushStylesNode->IsValueArray())
            {
            std::vector<wxBrushStyle> brushStyles;
            const auto brushStylesVals = brushStylesNode->GetValueArrayString();
            for (const auto& brushStylesVal : brushStylesVals)
                {
                const auto bStyle = ConvertBrushStyle(brushStylesVal);
                if (bStyle)
                    { brushStyles.push_back(bStyle.value()); }
                }
            const auto colorScheme = LoadColorScheme(brushSchemeNode->GetProperty(L"color-scheme"));
            if (colorScheme)
                {
                return std::make_shared<Brushes::Schemes::BrushScheme>(brushStyles, *colorScheme);
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Colors::Schemes::ColorScheme> ReportBuilder::LoadColorScheme(
        const wxSimpleJSON::Ptr_t& colorSchemeNode)
        {
        static const std::map<std::wstring_view, std::shared_ptr<Colors::Schemes::ColorScheme>> colorSchemes =
            {
            { L"dusk", std::make_shared<Colors::Schemes::Dusk>() },
            { L"earthtones", std::make_shared<Colors::Schemes::EarthTones>() },
            { L"decade1920s", std::make_shared<Colors::Schemes::Decade1920s>() },
            { L"decade1940s", std::make_shared<Colors::Schemes::Decade1940s>() },
            { L"decade1950s", std::make_shared<Colors::Schemes::Decade1950s>() },
            { L"decade1960s", std::make_shared<Colors::Schemes::Decade1960s>() },
            { L"decade1970s", std::make_shared<Colors::Schemes::Decade1970s>() },
            { L"decade1980s", std::make_shared<Colors::Schemes::Decade1980s>() },
            { L"decade1990s", std::make_shared<Colors::Schemes::Decade1990s>() },
            { L"decade2000s", std::make_shared<Colors::Schemes::Decade2000s>() },
            { L"october", std::make_shared<Colors::Schemes::October>() },
            { L"slytherin", std::make_shared<Colors::Schemes::Slytherin>() },
            { L"campfire", std::make_shared<Colors::Schemes::Campfire>() },
            { L"coffeeshop", std::make_shared<Colors::Schemes::CoffeeShop>() },
            { L"articchill", std::make_shared<Colors::Schemes::ArticChill>() },
            { L"backtoschool", std::make_shared<Colors::Schemes::BackToSchool>() },
            { L"boxofchocolates", std::make_shared<Colors::Schemes::BoxOfChocolates>() },
            { L"cosmopolitan", std::make_shared<Colors::Schemes::Cosmopolitan>() },
            { L"dayandnight", std::make_shared<Colors::Schemes::DayAndNight>() },
            { L"freshflowers", std::make_shared<Colors::Schemes::FreshFlowers>() },
            { L"icecream", std::make_shared<Colors::Schemes::IceCream>() },
            { L"urbanoasis", std::make_shared<Colors::Schemes::UrbanOasis>() },
            { L"typewriter", std::make_shared<Colors::Schemes::Typewriter>() },
            { L"tastywaves", std::make_shared<Colors::Schemes::TastyWaves>() },
            { L"spring", std::make_shared<Colors::Schemes::Spring>() },
            { L"shabbychic", std::make_shared<Colors::Schemes::ShabbyChic>() },
            { L"rollingthunder", std::make_shared<Colors::Schemes::RollingThunder>() },
            { L"producesection", std::make_shared<Colors::Schemes::ProduceSection>() },
            { L"nautical", std::make_shared<Colors::Schemes::Nautical>() },
            { L"semesters", std::make_shared<Colors::Schemes::Semesters>() },
            { L"seasons", std::make_shared<Colors::Schemes::Seasons>() },
            { L"meadowsunset", std::make_shared<Colors::Schemes::MeadowSunset>() }
            };
        
        if (!colorSchemeNode->IsOk())
            { return nullptr; }
        else if (colorSchemeNode->IsValueArray())
            {
            std::vector<wxColour> colors;
            const auto colorValues = colorSchemeNode->GetValueStringVector();
            if (colorValues.size() == 0)
                { return nullptr; }
            for (auto& color : colorValues)
                { colors.emplace_back(ConvertColor(color)); }
            return std::make_shared<Colors::Schemes::ColorScheme>(colors);
            }
        else if (colorSchemeNode->IsValueString())
            {
            const auto foundPos = colorSchemes.find(
                std::wstring_view(colorSchemeNode->GetValueString().MakeLower().wc_str()));
            if (foundPos != colorSchemes.cend())
                { return foundPos->second; }
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Wisteria::LineStyleScheme> ReportBuilder::LoadLineStyleScheme(
        const wxSimpleJSON::Ptr_t& lineStyleSchemeNode)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring_view, LineStyle> lineStyleEnsums =
            {
            { L"arrows", LineStyle::Arrows },
            { L"lines", LineStyle::Lines },
            { L"spline", LineStyle::Spline }
            };

        if (!lineStyleSchemeNode->IsOk())
            { return nullptr; }
        // a list of icons
        else if (lineStyleSchemeNode->IsValueArray())
            {
            std::vector<std::pair<wxPenStyle, LineStyle>> lineStyles;
            auto lineStyleValues = lineStyleSchemeNode->GetValueArrayObject();
            for (auto& lineStyle : lineStyleValues)
                {
                wxPen pn(*wxBLACK, 1, wxPenStyle::wxPENSTYLE_SOLID);
                LoadPen(lineStyle->GetProperty(L"pen-style"), pn);
                const auto foundPos = lineStyleEnsums.find(
                    std::wstring_view(lineStyle->GetProperty(L"line-style")->
                        GetValueString().MakeLower().wc_str()));
                if (foundPos != lineStyleEnsums.cend())
                    { lineStyles.emplace_back(pn.GetStyle(), foundPos->second); }
                }
            if (lineStyles.size() == 0)
                { return nullptr; }
            return std::make_shared<LineStyleScheme>(lineStyles);
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::optional<Icons::IconShape> ReportBuilder::ConvertIcon(wxString iconStr)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring_view, IconShape> iconEnums =
            {
            { L"blank", IconShape::Blank },
            { L"horizontal-line", IconShape::HorizontalLine },
            { L"arrow-right", IconShape::ArrowRight },
            { L"circle", IconShape::Circle },
            { L"image", IconShape::Image },
            { L"horizontal-separator", IconShape::HorizontalSeparator },
            { L"horizontal-arrow-right-separator", IconShape::HorizontalArrowRightSeparator },
            { L"color-gradient", IconShape::ColorGradient },
            { L"square", IconShape::Square },
            { L"triangle-upward", IconShape::TriangleUpward },
            { L"triangle-downward", IconShape::TriangleDownward },
            { L"triangle-right", IconShape::TriangleRight },
            { L"triangle-left", IconShape::TriangleLeft },
            { L"diamond", IconShape::Diamond },
            { L"plus", IconShape::Plus },
            { L"asterisk", IconShape::Asterisk },
            { L"hexagon", IconShape::Hexagon },
            { L"box-plot", IconShape::BoxPlot },
            { L"location-marker", IconShape::LocationMarker },
            { L"go-road-sign", IconShape::GoRoadSign },
            { L"warning-road-sign", IconShape::WarningRoadSign },
            { L"sun", IconShape::Sun },
            { L"flower", IconShape::Flower },
            { L"fall-leaf", IconShape::FallLeaf },
            { L"top-curly-brace", IconShape::TopCurlyBrace },
            { L"right-curly-brace", IconShape::RightCurlyBrace },
            { L"bottom-curly-brace", IconShape::BottomCurlyBrace },
            { L"left-curly-brace", IconShape::LeftCurlyBrace },
            { L"male", IconShape::Male },
            { L"female", IconShape::Female },
            { L"female-business", IconShape::FemaleBusiness }
            };

        const auto foundPos = iconEnums.find(std::wstring_view(iconStr.MakeLower().wc_str()));
        return (foundPos != iconEnums.cend() ?
                std::optional<Icons::IconShape>(foundPos->second) :
                std::nullopt);
        }

    //---------------------------------------------------
    std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> ReportBuilder::LoadIconScheme(
        const wxSimpleJSON::Ptr_t& iconSchemeNode)
        {
        static const std::map<std::wstring_view, std::shared_ptr<IconScheme>> iconSchemes =
            {
            { L"standard-shapes", std::make_shared<StandardShapes>() },
            { L"semesters", std::make_shared<Semesters>() }
            };

        if (!iconSchemeNode->IsOk())
            { return nullptr; }
        // a list of icons
        else if (iconSchemeNode->IsValueArray())
            {
            std::vector<IconShape> icons;
            auto iconValues = iconSchemeNode->GetValueStringVector();
            if (iconValues.size() == 0)
                { return nullptr; }
            for (auto& icon : iconValues)
                {
                const auto iconValue =  ConvertIcon(icon);
                if (iconValue.has_value())
                    { icons.emplace_back(iconValue.value()); }
                }
            if (icons.size() == 0)
                { return nullptr; }
            return std::make_shared<IconScheme>(icons);
            }
        // a pre-defined icon scheme
        else if (iconSchemeNode->IsValueString())
            {
            const auto foundPos = iconSchemes.find(
                std::wstring_view(iconSchemeNode->GetValueString().MakeLower().wc_str()));
            if (foundPos != iconSchemes.cend())
                { return foundPos->second; }
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Image> ReportBuilder::LoadImage(
        const wxSimpleJSON::Ptr_t& imageNode)
        {
        static const std::map<std::wstring_view, ResizeMethod> resizeValues =
            {
            { L"downscale-only", ResizeMethod::DownscaleOnly },
            { L"downscale-or-upscale", ResizeMethod::DownscaleOrUpscale },
            { L"upscale-only", ResizeMethod::UpscaleOnly },
            { L"no-resize", ResizeMethod::NoResize },
            };

        auto path = imageNode->GetProperty(L"path")->GetValueString();
        if (path.empty())
            {
            throw std::runtime_error(
                wxString(_(L"Image must have a filepath.")).ToUTF8());
            }
        if (!wxFileName::FileExists(path))
            {
            path = wxFileName(m_configFilePath).GetPathWithSep() + path;
            if (!wxFileName::FileExists(path))
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: image not found."), path).ToUTF8());
                }
            }
        auto image = std::make_shared<GraphItems::Image>(path);
        if (image->IsOk())
            {
            // center by default, but allow LoadItems (below) to override that
            // if client asked for something else
            image->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            image->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

            auto foundPos = resizeValues.find(std::wstring_view(
                imageNode->GetProperty(L"resize-method")->GetValueString().MakeLower().wc_str()));
            if (foundPos != resizeValues.cend())
                { image->SetResizeMethod(foundPos->second); }
            LoadItem(imageNode, image);

            image->SetFixedWidthOnCanvas(true);
            return image;
            }
        else
            { return nullptr; }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadItem(const wxSimpleJSON::Ptr_t& itemNode,
                                 std::shared_ptr<GraphItems::GraphItemBase> item)
        {
        if (!itemNode->IsOk() || item == nullptr)
            { return; }

        static const std::map<std::wstring_view, Anchoring> anchoringValues =
            {
            { L"bottom-left-corner", Anchoring::BottomLeftCorner },
            { L"bottom-right-corner", Anchoring::BottomRightCorner },
            { L"center", Anchoring::Center },
            { L"top-left-corner", Anchoring::TopLeftCorner },
            { L"top-right-corner", Anchoring::TopRightCorner },
            };

        item->SetDPIScaleFactor(m_dpiScaleFactor);

        // ID
        item->SetId(itemNode->GetProperty(L"id")->GetValueNumber(wxID_ANY));

        // anchoring
        const auto foundPos = anchoringValues.find(std::wstring_view(
            itemNode->GetProperty(L"anchoring")->GetValueString().MakeLower().wc_str()));
        if (foundPos != anchoringValues.cend())
            { item->SetAnchoring(foundPos->second); }

        // child-alignment
        const auto childPlacement = itemNode->GetProperty(L"relative-alignment")->GetValueString();
        if (childPlacement.CmpNoCase(L"flush-left") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushLeft); }
        else if (childPlacement.CmpNoCase(L"flush-right") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushRight); }
        else if (childPlacement.CmpNoCase(L"flush-top") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushTop); }
        else if (childPlacement.CmpNoCase(L"flush-bottom") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushBottom); }
        else if (childPlacement.CmpNoCase(L"centered") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::Centered); }

        // padding (going clockwise)
        const auto paddingSpec = itemNode->GetProperty(L"padding")->GetValueArrayNumber();
        if (paddingSpec.size() > 0)
            { item->SetTopPadding(paddingSpec.at(0)); }
        if (paddingSpec.size() > 1)
            { item->SetRightPadding(paddingSpec.at(1)); }
        if (paddingSpec.size() > 2)
            { item->SetBottomPadding(paddingSpec.at(2)); }
        if (paddingSpec.size() > 3)
            { item->SetLeftPadding(paddingSpec.at(3)); }

        // canvas padding (going clockwise)
        const auto canvasPaddingSpec = itemNode->GetProperty(L"canvas-margins")->GetValueArrayNumber();
        if (canvasPaddingSpec.size() > 0)
            { item->SetTopCanvasMargin(canvasPaddingSpec.at(0)); }
        if (canvasPaddingSpec.size() > 1)
            { item->SetRightCanvasMargin(canvasPaddingSpec.at(1)); }
        if (canvasPaddingSpec.size() > 2)
            { item->SetBottomCanvasMargin(canvasPaddingSpec.at(2)); }
        if (canvasPaddingSpec.size() > 3)
            { item->SetLeftCanvasMargin(canvasPaddingSpec.at(3)); }

        // horizontal page alignment
        const auto hPageAlignment = itemNode->GetProperty(L"horizontal-page-alignment")->GetValueString();
        if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
            { item->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned); }
        else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
            { item->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned); }
        else if (hPageAlignment.CmpNoCase(L"centered") == 0)
            { item->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered); }

        // vertical page alignment
        const auto vPageAlignment = itemNode->GetProperty(L"vertical-page-alignment")->GetValueString();
        if (vPageAlignment.CmpNoCase(L"top-aligned") == 0)
            { item->SetPageVerticalAlignment(PageVerticalAlignment::TopAligned); }
        else if (vPageAlignment.CmpNoCase(L"bottom-aligned") == 0)
            { item->SetPageVerticalAlignment(PageVerticalAlignment::BottomAligned); }
        else if (vPageAlignment.CmpNoCase(L"centered") == 0)
            { item->SetPageVerticalAlignment(PageVerticalAlignment::Centered); }

        item->SetScaling(itemNode->GetProperty(L"scaling")->GetValueNumber(1));

        LoadPen(itemNode->GetProperty(L"pen"), item->GetPen());

        item->SetFixedWidthOnCanvas(
            itemNode->GetProperty(L"fixed-width")->GetValueBool());
        item->FitCanvasRowHeightToContent(
            itemNode->GetProperty(L"fit-row-to-content")->GetValueBool());
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::FindAxisPosition(const GraphItems::Axis& axis,
        const wxSimpleJSON::Ptr_t& positionNode) const
        {
        std::optional<double> axisPos;
        if (positionNode->IsOk() && positionNode->IsValueString())
            {
            // see if it's a date
            wxDateTime dt;
            if (dt.ParseDateTime(positionNode->GetValueString()) ||
                dt.ParseDate(positionNode->GetValueString()))
                {
                axisPos = axis.FindDatePosition(dt);
                // looks like a date, but couldn't be found on the axis,
                // so just show it as a string
                if (!axisPos.has_value())
                    {
                    axisPos = axis.FindCustomLabelPosition(positionNode->GetValueString());
                    }
                }
            else
                {
                axisPos = axis.FindCustomLabelPosition(positionNode->GetValueString());
                }
            }
        else if (positionNode->IsOk() && positionNode->IsValueNumber())
            {
            axisPos = positionNode->GetValueNumber();
            }
        return axisPos;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                                  Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                                  std::shared_ptr<Graphs::Graph2D> graph)
        {
        LoadItem(graphNode, graph);

        // title information
        const auto titleProperty = graphNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            const auto titleLabel = LoadLabel(titleProperty, graph->GetTitle());
            if (titleLabel != nullptr)
                { graph->GetTitle() = *titleLabel; }
            }

        // subtitle information
        const auto subtitleProperty = graphNode->GetProperty(L"sub-title");
        if (subtitleProperty->IsOk())
            {
            const auto subtitleLabel = LoadLabel(subtitleProperty, graph->GetSubtitle());
            if (subtitleLabel != nullptr)
                { graph->GetSubtitle() = *subtitleLabel; }
            }

        // caption information
        const auto captionProperty = graphNode->GetProperty(L"caption");
        if (captionProperty->IsOk())
            {
            const auto captionLabel = LoadLabel(captionProperty, graph->GetCaption());
            if (captionLabel != nullptr)
                { graph->GetCaption() = *captionLabel; }
            }

        // axes
        const auto axesProperty = graphNode->GetProperty(L"axes");
        if (axesProperty->IsOk())
            {
            const auto axesNodes = axesProperty->GetValueArrayObject();
            for (const auto& axisNode : axesNodes)
                {
                const auto axisType = ConvertAxisType(
                    axisNode->GetProperty(L"axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    if (axisType.value() == AxisType::LeftYAxis)
                        { LoadAxis(axisNode, graph->GetLeftYAxis()); }
                    else if (axisType.value() == AxisType::RightYAxis)
                        { LoadAxis(axisNode, graph->GetRightYAxis()); }
                    else if (axisType.value() == AxisType::BottomXAxis)
                        { LoadAxis(axisNode, graph->GetBottomXAxis()); }
                    else if (axisType.value() == AxisType::TopXAxis)
                        { LoadAxis(axisNode, graph->GetTopXAxis()); }
                    }
                }
            }

        // annotations embedded on the plot
        const auto annotationNode = graphNode->GetProperty(L"annotations");
        if (annotationNode->IsOk())
            {
            const auto annotations = annotationNode->GetValueArrayObject();
            for (const auto& annotation : annotations)
                {
                auto label = LoadLabel(annotation->GetProperty(L"label"),
                                       GraphItems::Label());
                if (!label)
                    { continue; }
                // add outline and background color if not provided in config file
                if (!label->GetPen().IsOk())
                    { label->GetPen() = *wxBLACK_PEN; }
                if (!label->GetFontBackgroundColor().IsOk())
                    {
                    label->SetFontBackgroundColor(
                        ColorContrast::BlackOrWhiteContrast(label->GetFontColor()));
                    }
                label->SetPadding(5, 5, 5, 5);

                const auto interestPointsNode = annotation->GetProperty(L"interest-points");
                if (interestPointsNode->IsOk())
                    {
                    // get all the points on the plot that the note is pointing at
                    std::vector<wxPoint> interestPointPostions;
                    const auto interestPoints = interestPointsNode->GetValueArrayObject();
                    for (const auto& interestPoint : interestPoints)
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           interestPoint->GetProperty(L"x"));
                        const auto yPos = FindAxisPosition(graph->GetLeftYAxis(),
                                                           interestPoint->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            interestPointPostions.emplace_back(
                                wxPoint(xPos.value(), yPos.value()));
                            }
                        }
                    wxPoint anchorPt;
                    const auto anchorNode = annotation->GetProperty(L"anchor");
                    if (anchorNode->IsOk())
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           anchorNode->GetProperty(L"x"));
                        const auto yPos = FindAxisPosition(graph->GetLeftYAxis(),
                                                           anchorNode->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            anchorPt.x = xPos.value();
                            anchorPt.y = yPos.value();
                            }
                        }
                    // if no anchor point specified, then use the middle point of the interest points
                    /// @todo try to add even better logic in here, like how ggrepel works
                    else if (interestPointPostions.size())
                        {
                        const auto [minX, maxX] = std::minmax_element(
                            interestPointPostions.cbegin(),
                            interestPointPostions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.x < rhv.x; });
                        const auto [minY, maxY] = std::minmax_element(
                            interestPointPostions.cbegin(),
                            interestPointPostions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.y < rhv.y; });
                        anchorPt.x = safe_divide(maxX->x - minX->x, 2) + minX->x;
                        anchorPt.y = safe_divide(maxY->y - minY->y, 2) + minY->y;
                        }
                    if (anchorPt.IsFullySpecified())
                        { graph->AddAnnotation(label, anchorPt, interestPointPostions); }
                    }
                }
            }

        // reference lines
        const auto referenceLinesNode = graphNode->GetProperty(L"reference-lines");
        if (referenceLinesNode->IsOk())
            {
            const auto refLines = referenceLinesNode->GetValueArrayObject();
            for (const auto& refLine : refLines)
                {
                const auto axisType = ConvertAxisType(
                    refLine->GetProperty(L"axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    wxPen pen(*wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH);
                    LoadPen(refLine->GetProperty(L"pen"), pen);

                    auto& axis = (axisType == AxisType::BottomXAxis ?
                        graph->GetBottomXAxis() :
                        axisType == AxisType::TopXAxis ?
                        graph->GetTopXAxis() :
                        axisType == AxisType::LeftYAxis ?
                        graph->GetLeftYAxis() :
                        axisType == AxisType::RightYAxis ?
                        graph->GetRightYAxis() :
                        graph->GetBottomXAxis());
                    const auto axisPos =
                        FindAxisPosition(axis, refLine->GetProperty(L"position"));

                    if (axisPos.has_value())
                        {
                        graph->AddReferenceLine(
                            ReferenceLine(axisType.value(), axisPos.value(),
                                refLine->GetProperty(L"label")->GetValueString(),
                                pen));
                        }
                    }
                }
            }

        // reference areas
        static const std::map<std::wstring_view, ReferenceAreaStyle> refAreaValues =
            {
            { L"fade-from-left-to-right", ReferenceAreaStyle::FadeFromLeftToRight },
            { L"fade-from-right-to-left", ReferenceAreaStyle::FadeFromRightToLeft },
            { L"solid", ReferenceAreaStyle::Solid },
            };

        const auto referenceAreasNode = graphNode->GetProperty(L"reference-areas");
        if (referenceAreasNode->IsOk())
            {
            const auto refAreas = referenceAreasNode->GetValueArrayObject();
            for (const auto& refArea : refAreas)
                {
                const auto axisType = ConvertAxisType(
                    refArea->GetProperty(L"axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    wxPen pen(*wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH);
                    LoadPen(refArea->GetProperty(L"pen"), pen);

                    auto& axis = (axisType == AxisType::BottomXAxis ?
                        graph->GetBottomXAxis() :
                        axisType == AxisType::TopXAxis ?
                        graph->GetTopXAxis() :
                        axisType == AxisType::LeftYAxis ?
                        graph->GetLeftYAxis() :
                        axisType == AxisType::RightYAxis ?
                        graph->GetRightYAxis() :
                        graph->GetBottomXAxis());

                    ReferenceAreaStyle areaStyle{ ReferenceAreaStyle::Solid };
                    const auto foundPos = refAreaValues.find(
                        std::wstring_view(refArea->GetProperty(L"style")->
                            GetValueString().MakeLower().wc_str()));
                    if (foundPos != refAreaValues.cend())
                        { areaStyle = foundPos->second; }

                    const auto axisPos1 =
                        FindAxisPosition(axis, refArea->GetProperty(L"position-1"));

                    const auto axisPos2 =
                        FindAxisPosition(axis, refArea->GetProperty(L"position-2"));

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        graph->AddReferenceArea(
                            ReferenceArea(axisType.value(), axisPos1.value(), axisPos2.value(),
                                refArea->GetProperty(L"label")->GetValueString(),
                                pen, areaStyle));
                        }
                    }
                }
            }
        
        // is there a legend?
        const auto legendNode = graphNode->GetProperty(L"legend");
        if (legendNode->IsOk())
            {
            const auto ringPerimeterStr = legendNode->GetProperty(L"ring")->GetValueString();
            const auto ringPerimeter = (ringPerimeterStr.CmpNoCase(L"inner") == 0 ?
                                        Perimeter::Inner :
                                        Perimeter::Outer);
            const auto includeHeader = legendNode->GetProperty(L"include-header")->GetValueBool(true);
            const auto headerLabel = legendNode->GetProperty(L"title")->GetValueString();
            const auto placement = legendNode->GetProperty(L"placement")->GetValueString();
            std::shared_ptr<Label> legend{ nullptr };
            if (placement.CmpNoCase(L"left") == 0)
                {
                legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                canvas->SetFixedObject(currentRow, currentColumn+1, graph);
                canvas->SetFixedObject(currentRow, currentColumn++, legend);
                }
            else if (placement.CmpNoCase(L"bottom") == 0)
                {
                legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(++currentRow, currentColumn, legend);
                }
            else if (placement.CmpNoCase(L"top") == 0)
                {
                legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                canvas->SetFixedObject(currentRow+1, currentColumn, graph);
                canvas->SetFixedObject(currentRow++, currentColumn, legend);
                }
            else // right, the default
                {
                legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(currentRow, ++currentColumn, legend);
                }
            // update title
            if (legend && headerLabel.length())
                { legend->SetLine(0, headerLabel); }
            }
        // no legend, so just add the graph
        else
            {
            canvas->SetFixedObject(currentRow, currentColumn, graph);
            }
        }
    }