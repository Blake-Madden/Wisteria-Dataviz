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
#include "../data/dataset.h"
#include "../graphs/lineplot.h"
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
        std::shared_ptr<Graphs::Graph2D> LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                       Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                       std::shared_ptr<Graphs::Graph2D> graph);
        /// @brief Loads a line plot node.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        /// @todo many features still needed!
        std::shared_ptr<Graphs::Graph2D> LoadLinePlot(const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);
        /// @brief Converts a string value to an axis-type enum value.
        [[nodiscard]] static std::optional<AxisType> ConvertAxisType(const wxString& value);
        /// @brief Loads properties from a JSON node into an axis.
        /// @param axisNode The node to parse.
        /// @param axis[in,out] The axis to apply the loaded settings to.
        void LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis);

        /// @brief Loads a label node.
        /// @param labelNode The label node to parse.
        /// @returns A Label object, or null upon failure.
        /// @todo many features still needed!
        std::shared_ptr<GraphItems::Label> LoadLabel(const wxSimpleJSON::Ptr_t& labelNode);

        // the datasets used by all subitems in the report
        std::map<wxString, std::shared_ptr<Data::Dataset>, Data::StringCmpNoCase> m_datasets;
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
        };
    };

/** @}*/

#endif //__WISTERIA_REPORT_BUILDER_H__
