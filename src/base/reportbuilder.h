/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_REPORT_BUILDER_H__
#define __WISTERIA_REPORT_BUILDER_H__

#include "canvas.h"
#include "commonaxisbuilder.h"
#include "colorbrewer.h"
#include "fillableshape.h"
#include "../data/subset.h"
#include "../data/pivot.h"
#include "../data/join.h"
#include "../graphs/lineplot.h"
#include "../graphs/wcurveplot.h"
#include "../graphs/piechart.h"
#include "../graphs/categoricalbarchart.h"
#include "../graphs/histogram.h"
#include "../graphs/boxplot.h"
#include "../graphs/table.h"
#include "../graphs/lrroadmap.h"
#include "../graphs/proconroadmap.h"
#include "../graphs/heatmap.h"
#include "../graphs/ganttchart.h"
#include "../graphs/candlestickplot.h"
#include "../graphs/likertchart.h"
#include "../graphs/wordcloud.h"
#include "../wxSimpleJSON/src/wxSimpleJSON.h"
#include <vector>
#include <map>

namespace Wisteria
    {
    /** @brief Builds a report from a JSON configuration file.*/
    class ReportBuilder
        {
    public:
        /// @brief Loads a JSON configuration file and generates a report from it.
        /// @param filePath The file path to the JSON file.
        /// @param parent The parent window that manages the returns pages (i.e., canvas).\n
        ///     This would usually be a @c wxSplitterWindow that can cycle through the pages.
        /// @returns A vector of canvas, representing pages in a report.
        [[nodiscard]] std::vector<Canvas*> LoadConfigurationFile(
            const wxString& filePath, wxWindow* parent);
    private:
        using ValuesType = std::variant<wxString, double>;

        /// @brief Loads the datasets node into @c m_datasets.
        /// @details These datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param datasetNode The datasets node.
        void LoadDatasets(const wxSimpleJSON::Ptr_t& datasetsNode);
        /// @brief Loads the constants node into @c m_values.
        /// @details This is a key/value map used by objects throughout the report,
        ///     referencing them by name.
        /// @param constantsNode The node containing the constants' definitions.
        void LoadConstants(const wxSimpleJSON::Ptr_t& constantsNode);
        /// @brief Loads the subsets node into @c m_datasets.
        /// @details These (subset) datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param subsetsNode The subsets node.
        /// @param parentToSubset The dataset connected to the current dataset node.
        void LoadSubsets(const wxSimpleJSON::Ptr_t& subsetsNode,
                         const std::shared_ptr<const Data::Dataset>& parentToSubset);
        /// @brief Loads the pivots node into @c m_datasets.
        /// @details These (pivoted) datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param pivotsNode The pivots node.
        /// @param parentToPivot The dataset connected to the current dataset node.
        void LoadPivots(const wxSimpleJSON::Ptr_t& pivotsNode,
                        const std::shared_ptr<const Data::Dataset>& parentToPivot);
        /// @brief Loads the merges node into @c m_datasets.
        /// @details These (merged) datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param pivotsNode The pivots node.
        /// @param datasetToMerge The dataset connected to the current dataset node.
        void LoadMerges(const wxSimpleJSON::Ptr_t& mergesNode,
                        const std::shared_ptr<const Data::Dataset>& datasetToMerge);
        /// @brief Loads a common axis node into @c m_commonAxesPostions.
        /// @details Will not construct it into the grid, but will cache information
        ///     about the common axis until everything is constructed. This is then
        ///     used to actually create the common axis then.
        /// @param commonAxisNode The common axis node.
        /// @param currentRow The row in the canvas where the common axis will be placed.
        /// @param currentColumn The column in the canvas where the common axis will be placed.
        void LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                            const size_t currentRow, const size_t currentColumn);
        /// @brief Loads the base properties for a graph item
        /// @param itemNode The JSON node to parse.
        /// @param item[in,out] The item to write the properties to.
        void LoadItem(const wxSimpleJSON::Ptr_t& itemNode,
            std::shared_ptr<GraphItems::GraphItemBase> item);
        /// @brief Finalizes loading options for a graph.
        /// @details This will load general graph options from a node,
        ///     apply them to the graph, and add the graph (and possibly its legend) to the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed. 
        /// @param[in,out] graph The graph to apply the settings to.
        /// @warning The node's graph-specific loading function should be called first, then
        ///     this should be called to finalize it and add it to the canvas.
        /// @todo many features still needed!
        void LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                       Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                       std::shared_ptr<Graphs::Graph2D> graph);
        /// @brief Loads a line plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        /// @todo many features still needed!
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadLinePlot(
                        const wxSimpleJSON::Ptr_t& graphNode,
                        Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a w-curve plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadWCurvePlot(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a Likert chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadLikertChart(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a candlestick plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadCandlestickPlot(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a linear regression roadmap node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadLRRoadmap(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a pro and con roadmap node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadProConRoadmap(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a word cloud node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadWordCloud(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a gantt chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadGanttChart(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a pie chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadPieChart(
                        const wxSimpleJSON::Ptr_t& graphNode,
                        Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a box plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadBoxPlot(
            const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a categorical barchart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadCategoricalBarChart(
                        const wxSimpleJSON::Ptr_t& graphNode,
                        Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a histogram node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadHistogram(
                        const wxSimpleJSON::Ptr_t& graphNode,
                        Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads base-level settings for bar charts.
        /// @param graphNode The graph node to parse.
        /// @param graph The bar chart to load the base settings to.
        void LoadBarChart(const wxSimpleJSON::Ptr_t& graphNode,
                          std::shared_ptr<Graphs::BarChart> barChart);
        /// @brief Loads a heatmap node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadHeatMap(
                        const wxSimpleJSON::Ptr_t& graphNode,
                        Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a table node into the canvas.
        /// @param graphNode The table node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the table will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the table will be placed.
        /// @returns The table that was added to the canvas, or null upon failure.
        std::shared_ptr<Graphs::Graph2D> LoadTable(
                        const wxSimpleJSON::Ptr_t& graphNode,
                        Canvas* canvas, size_t& currentRow, size_t& currentColumn);

        [[nodiscard]] std::shared_ptr<GraphItems::Shape> LoadShape(
                        const wxSimpleJSON::Ptr_t& shapeNode);

        [[nodiscard]] std::shared_ptr<GraphItems::FillableShape> LoadFillableShape(
                        const wxSimpleJSON::Ptr_t& shapeNode);

        /// @brief Loads properties from a JSON node into an axis.
        /// @param axisNode The node to parse.
        /// @param axis[in,out] The axis to apply the loaded settings to.
        void LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis);

        /// @brief Loads properties from a JSON node into a pen.
        /// @param penNode The node to parse.
        /// @param pen[in,out] The pen to apply the loaded settings to.
        void LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen);

        /// @brief Loads properties from a JSON node into a brush.
        /// @param brushNode The node to parse.
        /// @param brush[in,out] The brush to apply the loaded settings to.
        void LoadBrush(const wxSimpleJSON::Ptr_t& brushNode, wxBrush& brush);

        /// @brief Loads a row or column position for a table from a node.
        /// @details This support loading the @c origin and @c offset properties.
        /// @param positionNode The node to parse.
        /// @param columnCount The column count in the table.\n
        ///     This is what the constant @c "last-column" is expanded into (minus one).
        /// @param columnRow The row count in the table.\n
        ///     This is what the constant @c "last-row" is expanded into (minus one).
        /// @param table The table being reviewed.
        /// @note column and row counts should be the table's original column and row
        ///     counts, prior to any aggregation columns being added.
        /// @returns The row or column position.
        [[nodiscard]] std::optional<size_t> LoadTablePosition(
            const wxSimpleJSON::Ptr_t& positionNode,
            const size_t columnCount,
            const size_t columnRow,
            std::shared_ptr<Graphs::Table> table);

        /// @brief Loads an image node.
        /// @param imageNode The image node to parse.
        /// @returns The image that was loaded, or null upon failure.
        [[nodiscard]] std::shared_ptr<GraphItems::Image> LoadImage(const wxSimpleJSON::Ptr_t& imageNode);

        /// @brief Loads a bitmap node.
        /// @param bmpNode The bitmap node to parse.
        /// @returns The bitmap that was loaded (call `IsOk()` to validate it).
        [[nodiscard]] wxBitmap LoadImageFile(const wxSimpleJSON::Ptr_t& bmpNode);

        /// @brief Loads a label node.
        /// @param labelNode The label node to parse.
        /// @param labelTemplate The template to copy default settings from
        ///     before loading properties.
        /// @returns A Label object, or null upon failure.
        [[nodiscard]] std::shared_ptr<GraphItems::Label> LoadLabel(
            const wxSimpleJSON::Ptr_t& labelNode,
            const GraphItems::Label& labelTemplate);

        /// @brief Loads a color scheme from a node.
        /// @param colorSchemeNode The node to parse.
        /// @returns The loaded color scheme, or null upon failure.
        [[nodiscard]] std::shared_ptr<Colors::Schemes::ColorScheme> LoadColorScheme(
            const wxSimpleJSON::Ptr_t& colorSchemeNode);

        /// @brief Loads a brush scheme from a node.
        /// @param brushSchemeNode The node to parse.
        /// @returns The loaded brush scheme, or null upon failure.
        [[nodiscard]] std::shared_ptr<Brushes::Schemes::BrushScheme> LoadBrushScheme(
            const wxSimpleJSON::Ptr_t& brushSchemeNode);

        /// @brief Loads an icon scheme from a node.
        /// @param iconSchemeNode The node to parse.
        /// @returns The loaded icon scheme, or null upon failure.
        [[nodiscard]] std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> LoadIconScheme(
            const wxSimpleJSON::Ptr_t& iconSchemeNode);

        /// @brief Loads a line style scheme from a node.
        /// @param lineStyleSchemeNode The node to parse.
        /// @returns The loaded line style scheme, or null upon failure.
        [[nodiscard]] std::shared_ptr<Wisteria::LineStyleScheme> LoadLineStyleScheme(
            const wxSimpleJSON::Ptr_t& lineStyleSchemeNode);

        /** @brief Loads additional transformation features and applies them to a dataset.
            @param dsNode The datasource node that the dataset was loaded from.
            @param[in,out] dataset The dataset apply the transformations to.*/
        void LoadDatasetTransformations(
            const wxSimpleJSON::Ptr_t& dsNode,
            std::shared_ptr<Data::Dataset>& dataset);

        /// @brief If @c path is fully specified, then returns @c path.
        ///     Otherwise, tries to return the path relative to the project file.
        [[nodiscard]] wxString NormalizeFilePath(const wxString& path);

        /// @brief Converts a string value to a @c wxPaperSize enum value.
        [[nodiscard]] static std::optional<wxPaperSize>
            ConvertPaperSize(const wxString& value);
        /// @brief Converts a string value to a LabelPlaceent enum value.
        [[nodiscard]] static std::optional<LabelPlacement>
            ConvertLabelPlacement(const wxString& value);
        /// @brief Converts a string value to an AxisType enum value.
        [[nodiscard]] static std::optional<AxisType>
            ConvertAxisType(const wxString& value);
        /// @brief Converts a string value to a BinLabelDisplay enum value.
        [[nodiscard]] static std::optional<BinLabelDisplay>
            ConvertBinLabelDisplay(const wxString& value);
        /// @brief Converts a string value to a Roadmap::LaneSeparatorStyle enum value.
        [[nodiscard]] static std::optional<Graphs::Roadmap::LaneSeparatorStyle>
            ConvertLaneSeparatorStyle(const wxString& value);
        /// @brief Converts a string value to a Roadmap::RoadStopTheme enum value.
        [[nodiscard]] static std::optional<Graphs::Roadmap::RoadStopTheme>
            ConvertRoadStopTheme(const wxString& value);
        /// @brief Converts a string value to a Roadmap::MarkerLabelDisplay enum value.
        [[nodiscard]] static std::optional<Graphs::Roadmap::MarkerLabelDisplay>
            ConvertMarkerLabelDisplay(const wxString& value);
        /// @brief Converts a string value to a Histogram::BinningMethod enum value.
        [[nodiscard]] static std::optional<Graphs::Histogram::BinningMethod>
            ConvertBinningMethod(const wxString& value);
        /// @brief Converts a string value to a Histogram::IntervalDisplay enum value.
        [[nodiscard]] static std::optional<Graphs::Histogram::IntervalDisplay>
            ConvertIntervalDisplay(const wxString& value);
        /// @brief Converts a string value to a RoundingMethod enum value.
        [[nodiscard]] static std::optional<RoundingMethod>
            ConvertRoundingMethod(const wxString& value);
        /// @brief Converts a string value to a BoxEffect enum value.
        [[nodiscard]] static std::optional<BoxEffect>
            ConvertBoxEffect(const wxString& value);
        /// @brief Converts a string value to a PieSliceEffect enum value.
        [[nodiscard]] static std::optional<PieSliceEffect>
            ConvertPieSliceEffect(const wxString& value);
        /// @brief Converts a string value to a Perimeter enum value.
        [[nodiscard]] static std::optional<Perimeter>
            ConvertPerimeter(const wxString& value);
        /// @brief Converts a string value to a @c wxBrushStyle enum value.
        [[nodiscard]] static std::optional<wxBrushStyle>
            ConvertBrushStyle(const wxString& value);
        /// @brief Converts a string value to a @c TableCellFormat enum value.
        [[nodiscard]] static std::optional<TableCellFormat>
            ConvertTableCellFormat(const wxString& value);
        /// @brief Converts a string value to a @c DateInterval enum value.
        [[nodiscard]] static std::optional<DateInterval>
            ConvertDateInterval(const wxString& value);
        /// @brief Converts a string value to a @c FiscalYear enum value.
        [[nodiscard]] static std::optional<FiscalYear>
            ConvertFiscalYear(const wxString& value);
        /// @brief Converts a string value to a `GanttChart::TaskLabelDisplay` enum value.
        [[nodiscard]] static std::optional<Wisteria::Graphs::GanttChart::TaskLabelDisplay>
            ConvertTaskLabelDisplay(const wxString& value);
        /// @brief Converts a string value to a `CandlestickPlot::PlotType` enum value.
        [[nodiscard]] static std::optional<Wisteria::Graphs::CandlestickPlot::PlotType>
            ConvertCandlestickPlotType(const wxString& value);
        /// @brief Converts a string value to a `LikertChart::LikertSurveyQuestionFormat` enum value.
        [[nodiscard]] static std::optional<Wisteria::Graphs::LikertChart::LikertSurveyQuestionFormat>
            ConvertLikertSurveyQuestionFormat(const wxString& value);
        /// @brief Loads a color from a string.
        /// @param colorStr The string to parse and convert into a color.
        /// @returns The loaded color. Check with @c IsOk() to verify that the color
        ///     was successfully loaded.
        [[nodiscard]] wxColour ConvertColor(wxString colorStr);
        /// @brief Loads a color from a string.
        /// @param colorNode A color node to parse.
        /// @returns The loaded color. Check with @c IsOk() to verify that the color
        ///     was successfully loaded.
        [[nodiscard]] wxColour ConvertColor(const wxSimpleJSON::Ptr_t& colorNode);
        /// @brief Converts a string value to an icon shape enum value.
        /// @param iconStr the string name of the icon.
        /// @returns The icon enum value if found, @c std::nullopt otherwise.
        [[nodiscard]] std::optional<Icons::IconShape> ConvertIcon(wxString iconStr);
        /// @brief Converts a string value to a TextAlignment enum value.
        [[nodiscard]] static std::optional<TextAlignment>
            ConvertTextAlignment(const wxString& value);
        /// @brief Finds a position on the axis based on the value from a node.
        [[nodiscard]] std::optional<double> FindAxisPosition(const GraphItems::Axis& axis,
            const wxSimpleJSON::Ptr_t& positionNode) const;

        /** @brief Expands embedded placeholders in strings into their values.
            @param str The full string to expand.
            @returns The original string, with any placeholders in it replaced
                with the user-defined values.*/
        [[nodiscard]] wxString ExpandConstants(wxString str) const;
        [[nodiscard]] std::optional<double> ExpandNumericConstant(wxString str) const;
        /// @todo needs support for ID and date columns
        void CalcFormulas(const wxSimpleJSON::Ptr_t& formulasNode,
                          const std::shared_ptr<const Data::Dataset>& dataset);
        [[nodiscard]] ValuesType CalcFormula(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        // can be a continous min/max or string (case insensitive)
        [[nodiscard]] ValuesType CalcMinMax(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] std::optional<double> CalcValidN(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] std::optional<double> CalcTotal(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] std::optional<double> CalcGrandTotal(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] std::optional<double> CalcGroupCount(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] std::optional<double> CalcGroupPercentDecimal(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] std::optional<wxString> CalcGroupPercent(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]] wxString CalcNow(const wxString& formula) const;
        [[nodiscard]] wxString FormatPageNumber(const wxString& formula) const;
        
        // helpers for builing formula regexes
        //------------------------------------
        [[nodiscard]] static wxString FunctionStartRegEx()
            { return L"(?i)^[ ]*"; }
        [[nodiscard]] static wxString OpeningParenthesisRegEx()
            { return L"[ ]*\\([ ]*"; }
        [[nodiscard]] static wxString ClosingParenthesisRegEx()
            { return L"[ ]*\\)"; }
        // a parameter that is either wrapped in tickmarks (usually a var name),
        // double curly braces (a subformula), or empty string (no parameter)
        [[nodiscard]] static wxString ColumnNameOrFormulaRegEx()
            { return L"(`[^`]+`|{{[^}]*}}|[[:space:]]*)"; }
        [[nodiscard]] static wxString NumberOrStringRegEx()
            { return L"(`[^`]+`|[[:digit:]]+)"; }
        [[nodiscard]] static wxString ParamSepatatorRegEx()
            { return L"[ ]*,[ ]*"; }
        // a parameter that is either wrapped in tickmarks or empty string (no parameter)
        [[nodiscard]] static wxString StringOrEmptyRegEx()
            { return L"(`[^`]+`|[[:space:]]*)"; }

        // Converts a formula parameter into a column name(s) or group value.
        // Arguments may be a hard-coded column name (which will be enclosed in tickmarks),
        // or another formula (enclosed in {{}}). If the latter, this will calculate
        // that formula (which can be a column selection function, column aggregate function,
        // or group value).
        [[nodiscard]] wxString ConvertColumnOrGroupParameter(wxString columnStr,
            const std::shared_ptr<const Data::Dataset>& dataset) const;

        // variable selection functions
        //-----------------------------

        /// @brief Converts a multiple column selection function into a vector of column names.
        [[nodiscard]] std::optional<std::vector<wxString>> ExpandColumnSelections(wxString var,
            const std::shared_ptr<const Data::Dataset>& dataset);
        /// @brief Converts a single column selection function into a column name.
        [[nodiscard]] wxString ExpandColumnSelection(const wxString& formula,
            const std::shared_ptr<const Data::Dataset>& dataset) const;

        // the datasets used by all subitems in the report
        std::map<wxString, std::shared_ptr<Data::Dataset>, Data::wxStringLessNoCase> m_datasets;
        std::map<wxString, ValuesType, Data::wxStringLessNoCase> m_values;
        wxString m_name;

        size_t m_pageNumber{ 1 };

        static std::map<std::wstring_view, Wisteria::Colors::Color> m_colorMap;

        // cached locations of common axes
        struct CommonAxisPlaceholder
            {
            AxisType m_axisType{ AxisType::RightYAxis };
            std::pair<size_t, size_t> m_gridPosition;
            std::vector<double> m_childrenIds; // double because that's how cJSON reads numbers
            bool m_commonPerpendicularAxis{ false };
            const wxSimpleJSON::Ptr_t m_node;
            };
        std::vector<CommonAxisPlaceholder> m_commonAxesPlaceholders;
        double m_dpiScaleFactor{ 1.0 };

        wxString m_configFilePath;
        };
    };

/** @}*/

#endif //__WISTERIA_REPORT_BUILDER_H__
