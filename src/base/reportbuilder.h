/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_BUILDER_H
#define WISTERIA_REPORT_BUILDER_H

#include "../base/reportenumconvert.h"
#include "../base/tablelink.h"
#include "../data/join.h"
#include "../graphs/boxplot.h"
#include "../graphs/ganttchart.h"
#include "../graphs/lineplot.h"
#include "../graphs/table.h"
#include "../wxSimpleJSON/src/wxSimpleJSON.h"
#include "canvas.h"
#include "colorbrewer.h"
#include "commonaxisbuilder.h"
#include "fillableshape.h"
#include <map>
#include <vector>

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
        [[nodiscard]]
        std::vector<Canvas*> LoadConfigurationFile(const wxString& filePath, wxWindow* parent);

        /// @returns The map of color names and their respective colors.
        [[nodiscard]]
        static const std::map<std::wstring_view, Wisteria::Colors::Color>& GetColorMap() noexcept
            {
            return m_colorMap;
            }

        /// @brief Import options associated with a dataset.
        struct DatasetImportOptions
            {
            /// @brief The file path the dataset was imported from.
            wxString m_filePath;
            /// @brief Explicit importer override (e.g., "csv", "tsv", "xlsx", "ods").
            ///     Empty means deduce from the file extension.
            wxString m_importer;
            /// @brief The worksheet to import (name or 1-based index).
            std::variant<wxString, size_t> m_worksheet{ static_cast<size_t>(1) };
            /// @brief Column type overrides and selections for the import.
            Data::Dataset::ColumnPreviewInfo m_columnPreviewInfo;
            /// @brief Additional import settings (delimiters, locale, etc.).
            Data::ImportInfo m_importInfo;
            };

        /// @brief The type of pivot transformation.
        enum class PivotType
            {
            Wider,
            Longer
            };

        /// @brief Options describing how a pivoted dataset was created
        ///     from a source dataset.
        struct DatasetPivotOptions
            {
            /// @brief The type of pivot (wider or longer).
            PivotType m_type{ PivotType::Wider };
            /// @brief The name of the source dataset that was pivoted.
            wxString m_sourceDatasetName;

            // wider options
            /// @brief Columns used as row identifiers (wider pivot).
            std::vector<wxString> m_idColumns;
            /// @brief Categorical column whose values become new column names (wider pivot).
            wxString m_namesFromColumn;
            /// @brief Columns whose values fill the new wide-format cells (wider pivot).
            std::vector<wxString> m_valuesFromColumns;
            /// @brief Separator between combined column name parts (wider pivot).
            wxString m_namesSep{ L"_" };
            /// @brief Prefix prepended to generated column names (wider pivot).
            wxString m_namesPrefix;
            /// @brief Fill value for cells with no corresponding data (wider pivot).
            double m_fillValue{ std::numeric_limits<double>::quiet_NaN() };

            // longer options
            /// @brief Columns carried through unchanged (longer pivot).
            std::vector<wxString> m_columnsToKeep;
            /// @brief Columns whose values were stacked into rows (longer pivot).
            std::vector<wxString> m_fromColumns;
            /// @brief Target grouping column name(s) (longer pivot).
            std::vector<wxString> m_namesTo;
            /// @brief Target value column name (longer pivot).
            wxString m_valuesTo;
            /// @brief Optional regex pattern for splitting column names (longer pivot).
            wxString m_namesPattern;
            };

        /// @brief A column rename specification.
        struct DatasetColumnRename
            {
            /// @brief The original column name (for literal rename).
            wxString m_name;
            /// @brief The new column name (for literal rename).
            wxString m_newName;
            /// @brief A regex pattern matching column names (for regex rename).
            wxString m_nameRe;
            /// @brief The replacement string (for regex rename).
            wxString m_newNameRe;
            };

        /// @brief A categorical column mutation specification.
        struct DatasetMutateCategoricalColumn
            {
            /// the source column to read values from.
            wxString m_sourceColumn;
            /// the target column to write mutated values to.
            wxString m_targetColumn;
            /// pattern/replacement pairs
            std::vector<std::pair<wxString, wxString>> m_replacements;
            };

        /// @brief A regular expression recode specification.
        struct DatasetRecodeRe
            {
            /// the column to apply the recode to.
            wxString m_column;
            /// the regular expression pattern to match.
            wxString m_pattern;
            /// the replacement string for matched patterns.
            wxString m_replacement;
            };

        /// @brief A collapse-by-minimum-frequency specification.
        struct DatasetCollapseMin
            {
            /// the column to collapse.
            wxString m_column;
            /// the minimum frequency threshold; categories below this are collapsed.
            double m_min{ 2 };
            /// the label to assign to collapsed categories.
            wxString m_otherLabel;
            };

        /// @brief A collapse-except specification.
        struct DatasetCollapseExcept
            {
            /// the column to collapse.
            wxString m_column;
            /// the labels to keep as-is (all others are collapsed).
            std::vector<wxString> m_labelsToKeep;
            /// the label to assign to collapsed categories.
            wxString m_otherLabel;
            };

        /// @brief A formula name/value pair (stored as raw strings
        ///     for round-trip serialization).
        struct DatasetFormulaInfo
            {
            /// the name of the formula (used as the constant name).
            wxString m_name;
            /// The original formula string (e.g., "MAX(`Semester`)")
            /// or numeric literal as string.
            wxString m_value;
            };

        /// @brief All transformation options for a single named dataset.
        /// @details Mirrors everything that can appear at any level
        ///     of the JSON dataset tree.
        struct DatasetTransformOptions
            {
            /// column rename specifications.
            std::vector<DatasetColumnRename> m_columnRenames;
            /// categorical column mutation specifications.
            std::vector<DatasetMutateCategoricalColumn> m_mutateCategoricalColumns;
            /// column selection expression (regex or comma-separated names).
            wxString m_columnsSelect;
            /// regular expression recode specifications.
            std::vector<DatasetRecodeRe> m_recodeREs;
            /// collapse-by-minimum-frequency specifications.
            std::vector<DatasetCollapseMin> m_collapseMins;
            /// collapse-except specifications.
            std::vector<DatasetCollapseExcept> m_collapseExcepts;
            /// formula definitions for computed constants.
            std::vector<DatasetFormulaInfo> m_formulas;
            /// whether to sort column names alphabetically after transforms.
            bool m_columnNamesSort{ false };
            };

        /// @brief A single column filter specification.
        struct DatasetFilterInfo
            {
            /// the column to filter on.
            wxString m_column;
            /// the comparison operator (e.g., "=", "!=", "<", ">").
            wxString m_operator{ L"=" };
            /// Raw string representations of the filter values
            /// (constants like "{{MaxFall}}" are preserved unexpanded).
            std::vector<wxString> m_values;
            };

        /// @brief Options describing how a subset was created.
        struct DatasetSubsetOptions
            {
            /// the name of the parent dataset this was subsetted from
            wxString m_sourceDatasetName;

            /// which filter type is active
            enum class FilterType
                {
                Single,
                And,
                Or,
                Section
                };

            /// which filter type is active.
            FilterType m_filterType{ FilterType::Single };

            /// for Single / And / Or filter types
            std::vector<DatasetFilterInfo> m_filters;

            /// for Section filter type
            wxString m_sectionColumn;
            /// the label marking the start of the section.
            wxString m_sectionStartLabel;
            /// the label marking the end of the section.
            wxString m_sectionEndLabel;
            /// whether to include the sentinel (start/end) labels in the subset.
            bool m_sectionIncludeSentinelLabels{ true };
            };

        /// @brief Options describing how a merge was created.
        struct DatasetMergeOptions
            {
            /// the name of the primary (left) dataset.
            wxString m_sourceDatasetName;
            /// the name of the secondary (right) dataset to merge with.
            wxString m_otherDatasetName;
            /// the merge type (e.g., "left-join-unique", "inner-join").
            wxString m_type{ L"left-join-unique" };
            /// left-column / right-column pairs
            std::vector<std::pair<wxString, wxString>> m_byColumns;
            /// the suffix appended to disambiguate duplicate column names.
            wxString m_suffix{ L".x" };
            };

        /// @returns The datasets in the report.
        [[nodiscard]]
        const std::map<wxString, std::shared_ptr<Data::Dataset>, Data::wxStringLessNoCase>&
        GetDatasets() const noexcept
            {
            return m_datasets;
            }

        /// @brief Adds a dataset to the report.
        /// @param name The name for the dataset.
        /// @param dataset The dataset to add.
        /// @param importOptions The import options used to load the dataset.
        void AddDataset(const wxString& name, const std::shared_ptr<Data::Dataset>& dataset,
                        const DatasetImportOptions& importOptions)
            {
            m_datasets[name] = dataset;
            m_datasetImportOptions[name] = importOptions;
            }

        /// @brief Adds a dataset to the report with default import options.
        /// @param name The name for the dataset.
        /// @param dataset The dataset to add.
        void AddDataset(const wxString& name, const std::shared_ptr<Data::Dataset>& dataset)
            {
            AddDataset(name, dataset, DatasetImportOptions{});
            }

        /// @returns The import options for all datasets.
        [[nodiscard]]
        const std::map<wxString, DatasetImportOptions, Data::wxStringLessNoCase>&
        GetDatasetImportOptions() const noexcept
            {
            return m_datasetImportOptions;
            }

        /// @brief Associates pivot options with a named dataset.
        /// @param name The name of the pivoted dataset.
        /// @param pivotOptions The options describing how it was created.
        void SetDatasetPivotOptions(const wxString& name, const DatasetPivotOptions& pivotOptions)
            {
            m_datasetPivotOptions[name] = pivotOptions;
            }

        /// @returns The pivot options for all pivoted datasets.
        [[nodiscard]]
        const std::map<wxString, DatasetPivotOptions, Data::wxStringLessNoCase>&
        GetDatasetPivotOptions() const noexcept
            {
            return m_datasetPivotOptions;
            }

        /// @brief Associates transform options with a named dataset.
        /// @param name The name of the dataset.
        /// @param transformOptions The options describing the transformations.
        void SetDatasetTransformOptions(const wxString& name,
                                        const DatasetTransformOptions& transformOptions)
            {
            m_datasetTransformOptions[name] = transformOptions;
            }

        /// @returns The transform options for all datasets.
        [[nodiscard]]
        const std::map<wxString, DatasetTransformOptions, Data::wxStringLessNoCase>&
        GetDatasetTransformOptions() const noexcept
            {
            return m_datasetTransformOptions;
            }

        /// @brief Associates subset options with a named dataset.
        /// @param name The name of the subsetted dataset.
        /// @param subsetOptions The options describing how it was created.
        void SetDatasetSubsetOptions(const wxString& name,
                                     const DatasetSubsetOptions& subsetOptions)
            {
            m_datasetSubsetOptions[name] = subsetOptions;
            }

        /// @returns The subset options for all subsetted datasets.
        [[nodiscard]]
        const std::map<wxString, DatasetSubsetOptions, Data::wxStringLessNoCase>&
        GetDatasetSubsetOptions() const noexcept
            {
            return m_datasetSubsetOptions;
            }

        /// @brief Associates merge options with a named dataset.
        /// @param name The name of the merged dataset.
        /// @param mergeOptions The options describing how it was created.
        void SetDatasetMergeOptions(const wxString& name, const DatasetMergeOptions& mergeOptions)
            {
            m_datasetMergeOptions[name] = mergeOptions;
            }

        /// @returns The merge options for all merged datasets.
        [[nodiscard]]
        const std::map<wxString, DatasetMergeOptions, Data::wxStringLessNoCase>&
        GetDatasetMergeOptions() const noexcept
            {
            return m_datasetMergeOptions;
            }

        /// @returns The top-level constants (name/value pairs from the
        ///     "constants" JSON section).
        [[nodiscard]]
        const std::vector<DatasetFormulaInfo>& GetConstants() const noexcept
            {
            return m_constants;
            }

        /// @brief Generates a dataset name that does not collide with any
        ///     existing dataset in the report.
        /// @param baseName The preferred name.
        /// @returns @c baseName if it is not already taken, otherwise
        ///     @c baseName appended with a numeric suffix (e.g., "(2)").
        [[nodiscard]]
        wxString GenerateUniqueDatasetName(const wxString& baseName) const
            {
            if (!m_datasets.contains(baseName))
                {
                return baseName;
                }

            for (int i = 2; i <= 1'000; ++i)
                {
                const wxString candidate = wxString::Format(L"%s (%d)", baseName, i);
                if (!m_datasets.contains(candidate))
                    {
                    return candidate;
                    }
                }

            return baseName;
            }

        /// @returns The name of the report.
        [[nodiscard]]
        const wxString& GetName() const noexcept
            {
            return m_name;
            }

        /// @returns The watermark label.
        [[nodiscard]]
        const wxString& GetWatermarkLabel() const noexcept
            {
            return m_watermarkLabel;
            }

        /// @returns The watermark color.
        [[nodiscard]]
        const wxColour& GetWatermarkColor() const noexcept
            {
            return m_watermarkColor;
            }

        /// @returns The print orientation.
        [[nodiscard]]
        wxPrintOrientation GetPrintOrientation() const noexcept
            {
            return m_printOrientation;
            }

        /// @returns The print paper size.
        [[nodiscard]]
        wxPaperSize GetPaperSize() const noexcept
            {
            return m_paperSize;
            }

      private:
        using ValuesType = std::variant<wxString, double>;

        static auto wxStringVectorToWstringVector(const std::vector<wxString>& inVec)
            {
            std::vector<std::wstring> outVec(inVec.size());
            std::ranges::transform(inVec, outVec.begin(),
                                   [](const auto& val) { return val.ToStdWstring(); });
            return outVec;
            }

        /// @brief Loads the datasets node into @c m_datasets.
        /// @details These datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param datasetsNode The datasets node.
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
        /// @param parentName The name of the source dataset being pivoted.
        void LoadPivots(const wxSimpleJSON::Ptr_t& pivotsNode,
                        const std::shared_ptr<const Data::Dataset>& parentToPivot,
                        const wxString& parentName);
        /// @brief Loads the merges node into @c m_datasets.
        /// @details These (merged) datasets are used by objects throughout the report,
        ///     referencing them by name.
        /// @param mergesNode The merges node.
        /// @param datasetToMerge The dataset connected to the current dataset node.
        void LoadMerges(const wxSimpleJSON::Ptr_t& mergesNode,
                        const std::shared_ptr<const Data::Dataset>& datasetToMerge);
        /// @brief Loads a common axis node into @c m_commonAxesPositions.
        /// @details Will not construct it into the grid, but will cache information
        ///     about the common axis until everything is constructed. This is then
        ///     used to actually create the common axis then.
        /// @param commonAxisNode The common axis node.
        /// @param currentRow The row in the canvas where the common axis will be placed.
        /// @param currentColumn The column in the canvas where the common axis will be placed.
        void LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode, size_t currentRow,
                            size_t currentColumn);
        /// @brief Loads the base properties for a graph item
        /// @param itemNode The JSON node to parse.
        /// @param item[in,out] The item to write the properties to.
        void LoadItem(const wxSimpleJSON::Ptr_t& itemNode, GraphItems::GraphItemBase& item) const;
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
        void LoadGraph(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas, size_t& currentRow,
                       size_t& currentColumn, const std::shared_ptr<Graphs::Graph2D>& graph);
        /// @brief Loads a line plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadLinePlot(const wxSimpleJSON::Ptr_t& graphNode,
                                                      Canvas* canvas, size_t& currentRow,
                                                      size_t& currentColumn);
        /// @brief Loads options such as showcasing into different types of line plots.
        /// @param graphNode The graph node to parse.
        /// @param[in,out] linePlot The line plot to load options into.
        /// @todo many features still needed!
        void LoadLinePlotBaseOptions(const wxSimpleJSON::Ptr_t& graphNode,
                                     Graphs::LinePlot* linePlot) const;
        /// @brief Loads a multi-series line plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D>
        LoadMultiSeriesLinePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a w-curve plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadWCurvePlot(const wxSimpleJSON::Ptr_t& graphNode,
                                                        Canvas* canvas, size_t& currentRow,
                                                        size_t& currentColumn);
        /// @brief Loads a Likert chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadLikertChart(const wxSimpleJSON::Ptr_t& graphNode,
                                                         Canvas* canvas, size_t& currentRow,
                                                         size_t& currentColumn);
        /// @brief Loads a candlestick plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadCandlestickPlot(const wxSimpleJSON::Ptr_t& graphNode,
                                                             Canvas* canvas, size_t& currentRow,
                                                             size_t& currentColumn);
        /// @brief Loads a linear regression roadmap node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadLRRoadmap(const wxSimpleJSON::Ptr_t& graphNode,
                                                       Canvas* canvas, size_t& currentRow,
                                                       size_t& currentColumn);
        /// @brief Loads a pro and con roadmap node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadProConRoadmap(const wxSimpleJSON::Ptr_t& graphNode,
                                                           Canvas* canvas, size_t& currentRow,
                                                           size_t& currentColumn);
        /// @brief Loads a word cloud node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadWordCloud(const wxSimpleJSON::Ptr_t& graphNode,
                                                       Canvas* canvas, size_t& currentRow,
                                                       size_t& currentColumn);
        /// @brief Loads a Sankey dialog into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadSankeyDiagram(const wxSimpleJSON::Ptr_t& graphNode,
                                                           Canvas* canvas, size_t& currentRow,
                                                           size_t& currentColumn);
        /// @brief Loads a Gantt chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadGanttChart(const wxSimpleJSON::Ptr_t& graphNode,
                                                        Canvas* canvas, size_t& currentRow,
                                                        size_t& currentColumn);
        /// @brief Loads a pie chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadPieChart(const wxSimpleJSON::Ptr_t& graphNode,
                                                      Canvas* canvas, size_t& currentRow,
                                                      size_t& currentColumn);
        /// @brief Loads a box plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadBoxPlot(const wxSimpleJSON::Ptr_t& graphNode,
                                                     Canvas* canvas, size_t& currentRow,
                                                     size_t& currentColumn);
        /// @brief Loads a categorical bar chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D>
        LoadCategoricalBarChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                size_t& currentRow, size_t& currentColumn);
        /// @brief Loads a histogram node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadHistogram(const wxSimpleJSON::Ptr_t& graphNode,
                                                       Canvas* canvas, size_t& currentRow,
                                                       size_t& currentColumn);
        /// @brief Loads a scatter plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadScatterPlot(const wxSimpleJSON::Ptr_t& graphNode,
                                                         Canvas* canvas, size_t& currentRow,
                                                         size_t& currentColumn);
        /// @brief Loads a bubble plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadBubblePlot(const wxSimpleJSON::Ptr_t& graphNode,
                                                        Canvas* canvas, size_t& currentRow,
                                                        size_t& currentColumn);
        /// @brief Loads a Chernoff faces plot node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadChernoffFaces(const wxSimpleJSON::Ptr_t& graphNode,
                                                           Canvas* canvas, size_t& currentRow,
                                                           size_t& currentColumn);
        /// @brief Loads base-level settings for bar charts.
        /// @param graphNode The graph node to parse.
        /// @param barChart The bar chart to load the base settings to.
        void LoadBarChart(const wxSimpleJSON::Ptr_t& graphNode,
                          const std::shared_ptr<Graphs::BarChart>& barChart) const;
        /// @brief Loads a heatmap node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> LoadHeatMap(const wxSimpleJSON::Ptr_t& graphNode,
                                                     Canvas* canvas, size_t& currentRow,
                                                     size_t& currentColumn);
        /// @brief Loads a win/loss sparkline node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> WinLossSparkline(const wxSimpleJSON::Ptr_t& graphNode,
                                                          Canvas* canvas, size_t& currentRow,
                                                          size_t& currentColumn);
        /// @brief Loads a waffle chart node into the canvas.
        /// @param graphNode The graph node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the graph will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the graph will be placed.
        /// @returns The graph that was added to the canvas, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Graphs::Graph2D> WaffleChart(const wxSimpleJSON::Ptr_t& graphNode,
                                                     Canvas* canvas, size_t& currentRow,
                                                     size_t& currentColumn);
        /// @brief Loads a table node into the canvas.
        /// @param tableNode The table node to parse.
        /// @param canvas The canvas to add the graph to.
        /// @param[in,out] currentRow The row in the canvas where the table will be placed.
        /// @param[in,out] currentColumn The column in the canvas where the table will be placed.
        /// @returns The table that was added to the canvas, or null upon failure.
        std::shared_ptr<Graphs::Graph2D> LoadTable(const wxSimpleJSON::Ptr_t& tableNode,
                                                   Canvas* canvas, size_t& currentRow,
                                                   size_t& currentColumn);

        [[nodiscard]]
        std::unique_ptr<GraphItems::Shape> LoadShape(const wxSimpleJSON::Ptr_t& shapeNode) const;

        [[nodiscard]]
        GraphItems::ShapeInfo LoadShapeInfo(const wxSimpleJSON::Ptr_t& shapeNode) const;

        [[nodiscard]]
        std::unique_ptr<GraphItems::FillableShape>
        LoadFillableShape(const wxSimpleJSON::Ptr_t& shapeNode) const;

        /// @brief Loads properties from a JSON node into an axis.
        /// @param axisNode The node to parse.
        /// @param axis[in,out] The axis to apply the loaded settings to.
        void LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis);

        /// @brief Loads properties from a JSON node into a pen.
        /// @param penNode The node to parse.
        /// @param pen[in,out] The pen to apply the loaded settings to.
        /// @param item Optional item to cache color templates on.
        /// @param propertyPrefix Optional property prefix for caching
        ///     (e.g., "pen"). If empty, defaults to "pen.color".
        void LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen,
                     GraphItems::GraphItemBase* item = nullptr,
                     const wxString& propertyPrefix = wxString{}) const;

        /// @brief Loads properties from a JSON node into a brush.
        /// @param brushNode The node to parse.
        /// @param brush[in,out] The brush to apply the loaded settings to.
        /// @param item Optional item to cache color templates on.
        /// @param propertyPrefix Optional property prefix for caching.
        void LoadBrush(const wxSimpleJSON::Ptr_t& brushNode, wxBrush& brush,
                       GraphItems::GraphItemBase* item = nullptr,
                       const wxString& propertyPrefix = wxString{}) const;

        /// @brief Loads a row or column position for a table from a node.
        /// @details This support loading the @c origin and @c offset properties.
        /// @param positionNode The node to parse.
        /// @param table The table being reviewed.
        /// @note column and row counts should be the table's original column and row
        ///     counts, prior to any aggregation columns being added.
        /// @returns The row or column position.
        [[nodiscard]]
        static std::optional<size_t> LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
                                                       std::shared_ptr<Graphs::Table> table);

        /// @brief Loads an image node.
        /// @param imageNode The image node to parse.
        /// @returns The image that was loaded, or null upon failure.
        [[nodiscard]]
        std::unique_ptr<GraphItems::Image> LoadImage(const wxSimpleJSON::Ptr_t& imageNode) const;

        /// @brief Loads a bitmap node.
        /// @param bmpNode The bitmap node to parse.
        /// @returns The bitmap that was loaded (call `IsOk()` to validate it).
        [[nodiscard]]
        wxBitmap LoadImageFile(const wxSimpleJSON::Ptr_t& bmpNode) const;

        /// @brief Loads a label node.
        /// @param labelNode The label node to parse.
        /// @param labelTemplate The template to copy default settings from
        ///     before loading properties.
        /// @returns A Label object, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> LoadLabel(const wxSimpleJSON::Ptr_t& labelNode,
                                                     const GraphItems::Label& labelTemplate) const;

        /// @brief Loads a spacer node.
        /// @returns An empty Label object to simply consume space on the canvas.
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> LoadSpacer() const;

        /// @brief Loads an empty spacer node.
        /// @returns An empty Label object that just fills empty slots in a canvas row.
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> LoadEmptySpacer() const;

        /// @brief Loads a color scheme from a node.
        /// @param colorSchemeNode The node to parse.
        /// @returns The loaded color scheme, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Colors::Schemes::ColorScheme>
        LoadColorScheme(const wxSimpleJSON::Ptr_t& colorSchemeNode) const;

        /// @brief Loads a brush scheme from a node.
        /// @param brushSchemeNode The node to parse.
        /// @returns The loaded brush scheme, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<Brushes::Schemes::BrushScheme>
        LoadBrushScheme(const wxSimpleJSON::Ptr_t& brushSchemeNode) const;

        /// @brief Loads an icon scheme from a node.
        /// @param iconSchemeNode The node to parse.
        /// @returns The loaded icon scheme, or null upon failure.
        [[nodiscard]]
        static std::shared_ptr<Icons::Schemes::IconScheme>
        LoadIconScheme(const wxSimpleJSON::Ptr_t& iconSchemeNode);

        /// @brief Loads a line style scheme from a node.
        /// @param lineStyleSchemeNode The node to parse.
        /// @returns The loaded line style scheme, or null upon failure.
        [[nodiscard]]
        std::shared_ptr<LineStyleScheme>
        LoadLineStyleScheme(const wxSimpleJSON::Ptr_t& lineStyleSchemeNode) const;

        /** @brief Loads additional transformation features and applies them to a dataset.
            @param dsNode The datasource node that the dataset was loaded from.
            @param[in,out] dataset The dataset apply the transformations to.*/
        void LoadDatasetTransformations(const wxSimpleJSON::Ptr_t& dsNode,
                                        const std::shared_ptr<Data::Dataset>& dataset);

        /// @brief Finds a position on the axis based on the value from a node.
        [[nodiscard]]
        std::optional<double> FindAxisPosition(const GraphItems::Axis& axis,
                                               const wxSimpleJSON::Ptr_t& positionNode) const;

        /// @brief If @c path is fully specified, then returns @c path.
        ///     Otherwise, tries to return the path relative to the project file.
        [[nodiscard]]
        wxString NormalizeFilePath(const wxString& path) const;

        /// @brief Loads the positions from a row or column stops array.
        [[nodiscard]]
        auto LoadTableStops(std::shared_ptr<Graphs::Table>& table, const auto& stopsNode)
            {
            std::set<size_t> rowOrColumnStops;
            const auto stops = stopsNode->AsNodes();
            if (stops.size())
                {
                for (const auto& stop : stops)
                    {
                    const std::optional<size_t> stopPosition =
                        LoadTablePosition(stop->GetProperty(L"position"), table);
                    if (stopPosition.has_value())
                        {
                        rowOrColumnStops.insert(stopPosition.value());
                        }
                    }
                }
            return rowOrColumnStops;
            }

        /// @brief Reads a single position and range of positions (start and end).
        [[nodiscard]]
        auto ReadPositions(std::shared_ptr<Graphs::Table>& table,
                           const wxSimpleJSON::Ptr_t& theNode)
            {
            const std::optional<size_t> position =
                LoadTablePosition(theNode->GetProperty(L"position"), table);
            const std::optional<size_t> startPosition =
                LoadTablePosition(theNode->GetProperty(L"start"), table);
            const std::optional<size_t> endPosition =
                LoadTablePosition(theNode->GetProperty(L"end"), table);
            return std::tuple(position, startPosition, endPosition);
            }

        void LoadTableRowFormatting(std::shared_ptr<Graphs::Table>& table,
                                    const wxSimpleJSON::Ptr_t& tableNode);

        void LoadTableColumnFormatting(std::shared_ptr<Graphs::Table>& table,
                                       const wxSimpleJSON::Ptr_t& tableNode);

        /// @brief Loads a color from a string.
        /// @param colorStr The string to parse and convert into a color.
        /// @param item Optional item to cache the color template on.
        /// @param property Optional property name for caching
        ///     (e.g., "pen.color", "brush.color").
        /// @returns The loaded color. Check with @c IsOk() to verify that the color
        ///     was successfully loaded.
        [[nodiscard]]
        wxColour ConvertColor(wxString colorStr, GraphItems::GraphItemBase* item = nullptr,
                              const wxString& property = wxString{}) const;

        /// @brief Loads a color from a string.
        /// @param colorNode A color node to parse.
        /// @returns The loaded color. Check with @c IsOk() to verify that the color
        ///     was successfully loaded.
        [[nodiscard]]
        wxColour ConvertColor(const wxSimpleJSON::Ptr_t& colorNode) const;

        [[nodiscard]]
        std::optional<double> ConvertNumber(const wxSimpleJSON::Ptr_t& node) const
            {
            if (node->IsValueNumber())
                {
                return node->AsDouble();
                }
            if (node->IsValueString())
                {
                return ExpandNumericConstant(node->AsString());
                }
            return std::nullopt;
            }

        /** @brief Expands embedded placeholders in strings into their values.
            @param str The full string to expand.
            @returns The original string, with any placeholders in it replaced
                with the user-defined values.*/
        [[nodiscard]]
        wxString ExpandConstants(wxString str) const;

        [[nodiscard]]
        std::vector<wxString> ExpandConstants(std::vector<wxString> strs) const
            {
            for (auto& str : strs)
                {
                str = ExpandConstants(str);
                }
            return strs;
            }

        /** @brief Expands constants in a string and caches the original
                template on the item if it contained placeholders.
            @param item The item to cache the template on (can be @c nullptr).
            @param property The property name to cache under.
            @param rawValue The raw string that may contain \{\{\}\} placeholders.
            @returns The expanded string.*/
        [[nodiscard]]
        wxString ExpandAndCache(GraphItems::GraphItemBase* item, const wxString& property,
                                const wxString& rawValue) const
            {
            const wxString expanded = ExpandConstants(rawValue);
            if (expanded != rawValue && item != nullptr)
                {
                item->SetPropertyTemplate(property, rawValue);
                }
            return expanded;
            }

        /** @brief Expands constants in a vector of strings and caches
                the original templates on the item with indexed property names.
            @param item The item to cache the templates on (can be @c nullptr).
            @param property The property base name (elements cached as
                "property[0]", "property[1]", etc.).
            @param rawValues The raw strings that may contain \{\{\}\} placeholders.
            @returns The expanded strings.*/
        [[nodiscard]]
        std::vector<wxString> ExpandAndCache(GraphItems::GraphItemBase* item,
                                             const wxString& property,
                                             const std::vector<wxString>& rawValues) const
            {
            std::vector<wxString> expanded;
            expanded.reserve(rawValues.size());
            for (size_t i = 0; i < rawValues.size(); ++i)
                {
                expanded.push_back(ExpandAndCache(item, property + L"[" + std::to_wstring(i) + L"]",
                                                  rawValues[i]));
                }
            return expanded;
            }

        [[nodiscard]]
        std::optional<double> ExpandNumericConstant(wxString str) const;
        /// @todo needs support for ID and date columns
        void CalcFormulas(const wxSimpleJSON::Ptr_t& formulasNode,
                          const std::shared_ptr<const Data::Dataset>& dataset);
        [[nodiscard]]
        ValuesType CalcFormula(const wxString& formula,
                               const std::shared_ptr<const Data::Dataset>& dataset) const;
        // can be a continuous min/max or string (case-insensitive)
        [[nodiscard]]
        ValuesType CalcMinMax(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        std::optional<double> CalcValidN(const wxString& formula,
                                         const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        std::optional<double> CalcMedian(const wxString& formula,
                                         const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        std::optional<double> CalcTotal(const wxString& formula,
                                        const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        static std::optional<double>
        CalcGrandTotal(const wxString& formula,
                       const std::shared_ptr<const Data::Dataset>& dataset);
        [[nodiscard]]
        std::optional<double>
        CalcGroupCount(const wxString& formula,
                       const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        std::optional<double>
        CalcGroupPercentDecimal(const wxString& formula,
                                const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        std::optional<double>
        CalcGroupPercent(const wxString& formula,
                         const std::shared_ptr<const Data::Dataset>& dataset) const;
        [[nodiscard]]
        static wxString CalcNow(const wxString& formula);

        [[nodiscard]]
        wxString CalcAdd(const wxString& formula) const;

        [[nodiscard]]
        wxString FormatPageNumber(const wxString& formula) const;

        // helpers for building formula regexes
        //------------------------------------
        [[nodiscard]]
        static wxString FunctionStartRegEx()
            {
            return L"(?i)^[ ]*";
            }

        [[nodiscard]]
        static wxString OpeningParenthesisRegEx()
            {
            return L"[ ]*\\([ ]*";
            }

        [[nodiscard]]
        static wxString ClosingParenthesisRegEx()
            {
            return L"[ ]*\\)";
            }

        // a parameter that is either wrapped in tick marks (usually a var name),
        // double curly braces (a sub-formula), or empty string (no parameter)
        [[nodiscard]]
        static wxString ColumnNameOrFormulaRegEx()
            {
            return L"(`[^`]+`|{{[^}]*}}|[[:space:]]*)";
            }

        [[nodiscard]]
        static wxString NumberOrStringRegEx()
            {
            return L"(`[^`]+`|[[:digit:] ]+)";
            }

        [[nodiscard]]
        static wxString ParamSeparatorRegEx()
            {
            return L"[ ]*,[ ]*";
            }

        // a parameter that is either wrapped in tick marks or empty string (no parameter)
        [[nodiscard]]
        static wxString StringOrEmptyRegEx()
            {
            return L"(`[^`]+`|[[:space:]]*)";
            }

        // Converts a formula parameter into a column name(s) or group value.
        // Arguments may be a hard-coded column name (which will be enclosed in tick marks),
        // or another formula (enclosed in {{}}). If the latter, this will calculate
        // that formula (which can be a column selection function, column aggregate function,
        // or group value).
        [[nodiscard]]
        wxString
        ConvertColumnOrGroupParameter(wxString columnStr,
                                      const std::shared_ptr<const Data::Dataset>& dataset) const;

        // variable selection functions
        //-----------------------------

        /// @brief Converts a multiple column selection function into a vector of column names.
        [[nodiscard]]
        std::optional<std::vector<wxString>>
        ExpandColumnSelections(wxString var,
                               const std::shared_ptr<const Data::Dataset>& dataset) const;
        /// @brief Converts a single column selection function into a column name.
        [[nodiscard]]
        static wxString ExpandColumnSelection(const wxString& formula,
                                              const std::shared_ptr<const Data::Dataset>& dataset);

        // the datasets used by all subitems in the report
        std::map<wxString, std::shared_ptr<Data::Dataset>, Data::wxStringLessNoCase> m_datasets;
        std::map<wxString, DatasetImportOptions, Data::wxStringLessNoCase> m_datasetImportOptions;
        std::map<wxString, DatasetPivotOptions, Data::wxStringLessNoCase> m_datasetPivotOptions;
        std::map<wxString, DatasetTransformOptions, Data::wxStringLessNoCase>
            m_datasetTransformOptions;
        std::map<wxString, DatasetSubsetOptions, Data::wxStringLessNoCase> m_datasetSubsetOptions;
        std::map<wxString, DatasetMergeOptions, Data::wxStringLessNoCase> m_datasetMergeOptions;
        std::vector<DatasetFormulaInfo> m_constants;
        std::map<wxString, ValuesType, Data::wxStringLessNoCase> m_values;
        wxString m_name;
        wxString m_watermarkLabel;
        wxColour m_watermarkColor;
        wxPrintOrientation m_printOrientation{ wxPrintOrientation::wxPORTRAIT };
        wxPaperSize m_paperSize{ wxPaperSize::wxPAPER_NONE };

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

        std::vector<Wisteria::TableLink> m_tableLinks;

        wxString m_configFilePath;
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_BUILDER_H
