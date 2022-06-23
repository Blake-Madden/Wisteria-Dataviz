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
        /// @brief Finalizes loading options for graph.
        /// @details This will load general graph options from a node,
        ///     apply them to the graph, and add the graph (and possibly its legend)
        ///     to the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed. 
        /// @param graph The graph to apply the settings to.
        /// @warning The node's graph-specific loading function should be called first, then
        ///     this should be called to finalize it and add it to the canvas.
        /// @todo many features still needed!
        void LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                       Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                       std::shared_ptr<Graphs::Graph2D> graph);
        /// @brief Loads a line plot node.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @todo many features still needed!
        void LoadLinePlot(const wxSimpleJSON::Ptr_t& graphNode,
            Canvas* canvas, size_t& currentRow, size_t& currentColumn);

        // the datasets used by all subitems in the report
        std::map<wxString, std::shared_ptr<Data::Dataset>, Data::StringCmpNoCase> m_datasets;
        wxString m_name;
        };
    };

/** @}*/

#endif //__WISTERIA_REPORT_BUILDER_H__
