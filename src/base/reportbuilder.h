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
#include "../data/dataset.h"
#include "../graphs/lineplot.h"
#include "../graphs/piechart.h"
#include "../graphs/categoricalbarchart.h"
#include "../graphs/table.h"
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
        /// @brief Loads the datasources node into @c m_datasets.
        /// @details These datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param datasourcesNode The datasources node.
        void LoadDatasources(const wxSimpleJSON::Ptr_t& datasourcesNode);
        /// @brief Loads the values node into @c m_values.
        /// @details This is a key/value mape used by objects throughout the report,
        ///     referencing them by name.
        /// @param valuesNode The values node.
        void LoadValues(const wxSimpleJSON::Ptr_t& valuesNode);
        /// @brief Loads a common axis node into @c m_commonAxesPostions.
        /// @details Will not construct it into the grid, but will cache information
        ///     about the common axis until everything is constructed. This is then
        ///     used to actually create the common axis then.
        /// @param commonAxisNode The common axis node.
        /// @param currentRow The row in the canvas where the common axis will be placed.
        /// @param currentColumn The column in the canvas where the common axis  will be placed.
        void LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                            const size_t currentRow, const size_t currentColumn);
        /// @brief Loads the base properties for a graph item
        /// @param itemNode The JSON node to parse.
        /// @param item[in,out] The item to write the properties to.
        void LoadItem(const wxSimpleJSON::Ptr_t& itemNode,
            std::shared_ptr<GraphItems::GraphItemBase> item);
        /// @brief Finalizes loading options for graph.
        /// @details This will load general graph options from a node,
        ///     apply them to the graph, and add the graph (and possibly its legend)
        ///     to the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed. 
        /// @param graph The graph to apply the settings to.
        /// @returns The graph that was added to the canvas, or null upon failure.
        /// @warning The node's graph-specific loading function should be called first, then
        ///     this should be called to finalize it and add it to the canvas.
        /// @todo many features still needed!
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                       Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                       std::shared_ptr<Graphs::Graph2D> graph);
        /// @brief Loads a line plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        /// @todo many features still needed!
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadLinePlot(const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a pie chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadPieChart(const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a categorical barchart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        /// @todo many features still needed!
        [[nodiscard]] std::shared_ptr<Graphs::Graph2D> LoadCategoricalBarChart(const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a table node into the canvas.
        /// @param graphNode The table node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the table will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the table will be placed.
        /// @returns The table that was added to the canvas, or null upon failure.
        /// @todo many features still needed!
        std::shared_ptr<Graphs::Graph2D> LoadTable(const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);

        /// @brief Loads properties from a JSON node into an axis.
        /// @param axisNode The node to parse.
        /// @param axis[in,out] The axis to apply the loaded settings to.
        void LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis);

        /// @brief Loads properties from a JSON node into a pen.
        /// @param penNode The node to parse.
        /// @param pen[in,out] The pen to apply the loaded settings to.
        void LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen);

        /// @brief Loads a row or column position from a node.
        /// @details This support loading the @c origin and @c offset properties.
        /// @param positionNode The node to parse.
        /// @param columnCount The column count in the table.\n
        ///     This is what the constant @c "last-column" is expanded into (minus one).
        /// @param columnRow The row count in the table.\n
        ///     This is what the constant @c "last-row" is expanded into (minus one).
        /// @returns The row or column position.
        [[nodiscard]] std::optional<size_t> LoadPosition(const wxSimpleJSON::Ptr_t& positionNode,
            const size_t columnCount,
            const size_t columnRow);

        /// @brief Loads a image node into the canvas.
        /// @param imageNode The image node to parse.
        /// @param canvas The canvas to add the image to.
        /// @param[in,out] currentRow The row in the canvas where the image will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the image will be placed.
        /// @returns The image that was added to the canvas, or null upon failure.
        /// @todo many features still needed!
        std::shared_ptr<GraphItems::Image> LoadImage(const wxSimpleJSON::Ptr_t& imageNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);

        /// @brief Loads a label node.
        /// @param labelNode The label node to parse.
        /// @param labelTemplate The template to copy defeault settings from
        ///     before loading properties.
        /// @returns A Label object, or null upon failure.
        /// @todo many features still needed!
        [[nodiscard]] std::shared_ptr<GraphItems::Label> LoadLabel(
            const wxSimpleJSON::Ptr_t& labelNode,
            const GraphItems::Label labelTemplate);

        /// @brief Loads a color scheme from a node.
        /// @param colorSchemeNode Tne node to parse.
        /// @return The loaded color scheme, or null upon failure.
        [[nodiscard]] std::shared_ptr<Colors::Schemes::ColorScheme> LoadColorScheme(
            const wxSimpleJSON::Ptr_t& colorSchemeNode);

        /// @brief Loads an icon scheme from a node.
        /// @param iconSchemeNode Tne node to parse.
        /// @return The loaded icon scheme, or null upon failure.
        [[nodiscard]] std::shared_ptr<IconScheme> LoadIconScheme(
            const wxSimpleJSON::Ptr_t& iconSchemeNode);

        /// @brief Converts a string value to a LabelPlaceent enum value.
        [[nodiscard]] static std::optional<LabelPlacement>
            ConvertLabelPlacement(const wxString& value);
        /// @brief Converts a string value to an AxisType enum value.
        [[nodiscard]] static std::optional<AxisType>
            ConvertAxisType(const wxString& value);
        /// @brief Converts a string value to a BinLabelDisplay enum value.
        [[nodiscard]] static std::optional<BinLabelDisplay>
            ConvertBinLabelDisplay(const wxString& value);
        /// @brief Loads a color from a string.
        /// @param colorStr The string to parse and converted into a color.
        /// @returns The loaded color. Check with @c IsOk() to verify that the color
        ///     was successfully loaded.
        [[nodiscard]] static wxColour ConvertColor(wxString colorStr);

        /** @brief Expands embedded placeholders in strings into their values.
            @param str The full string to expand.
            @returns The original string, with any placeholders in it replaced
                with the user-defined values.*/
        [[nodiscard]] wxString ExpandValues(wxString str) const;
        [[nodiscard]] wxString CalcFormula(const wxString& formula);
        [[nodiscard]] wxString CalcMinMaxValue(const wxString& formula);
        [[nodiscard]] wxString CalcValidNValue(const wxString& formula);

        // the datasets used by all subitems in the report
        std::map<wxString, std::shared_ptr<Data::Dataset>, Data::StringCmpNoCase> m_datasets;
        using ValuesType = std::variant<wxString, double>;
        std::map<wxString, ValuesType, Data::StringCmpNoCase> m_values;
        wxString m_name;

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
