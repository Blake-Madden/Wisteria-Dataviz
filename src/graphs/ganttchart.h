/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_GANTT_H__
#define __WISTERIA_GANTT_H__

#include "barchart.h"
#include "../base/colorbrewer.h"

namespace Wisteria::Graphs
    {
    /** @brief A chart which shows the progress of events (e.g., tasks) along a timeline.
        @details These are useful for project management.
        @image html GanttChart.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where one categorical column is the task name and two date columns
         represent the start and end dates. Optionally, categorical columns can specify a description for the
         task, as well whom the task is assigned to. Finally, an optional continuous column can specify
         the percent of how complete the task is.

         | Task            | Start      | End        | Description          | Resource    | Completion |
         | :--             | --:        | --:        | :--                  | :--         | --:        |
         | Acquisition     | 2022-07-01 | 8/31/2022  |                      | Management  | 100        |
         | Develop Product | 2022-09-01 | 12/25/2022 |                      | Development | 50         |
         | Testing         | 2022-10-15 | 12/25/2022 | Maybe outsource this | QA          |            |

         Note that if the start date is missing data, then the starting point of the task will be whatever
         the earliest date along the bottom axis. Likewise, if the end date is missing data, then the task
         will be drawn as an arrow, stretching to the end of the bottom axis.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 2);

         auto companyAcquisitionData = std::make_shared<Data::Dataset>();
         try
            {
            companyAcquisitionData->ImportCSV(L"datasets/Company Acquisition.csv",
                ImportInfo().
                ContinuousColumns({ L"Completion" }).
                DateColumns({ { L"Start" }, { L"End" } }).
                CategoricalColumns({
                    { L"Task" },
                    { L"Description" },
                    { L"Resource" }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

         auto ganttChart = std::make_shared<GanttChart>(canvas);
         ganttChart->SetData(companyAcquisitionData,
            DateInterval::FiscalQuarterly, FiscalYear::USBusiness,
            L"Task", L"Start", "End",
            // these columns are optional
            L"Resource", L"Description", L"Completion", L"Resource");

         // add deadlines
         auto releaseDate = ganttChart->GetScalingAxis().GetPointFromDate(
            wxDateTime(25, wxDateTime::Dec, 2022));
         if (releaseDate)
            {
            ganttChart->AddReferenceLine(ReferenceLine(AxisType::BottomXAxis,
                releaseDate.value(), _(L"Release"),
                ColorBrewer::GetColor(Colors::Color::TractorRed)) );
            }

         auto updateReleaseDate = ganttChart->GetScalingAxis().GetPointFromDate(
            wxDateTime(15, wxDateTime::Mar, 2023));
         if (updateReleaseDate)
            {
            ganttChart->AddReferenceLine(ReferenceLine(AxisType::BottomXAxis,
                updateReleaseDate.value(),
                _(L"Update Release"),
                ColorBrewer::GetColor(Colors::Color::TractorRed,
                                      Wisteria::Settings::GetTranslucencyValue())));
            }

         ganttChart->SetCanvasMargins(5, 5, 5, 5);
         canvas->SetFixedObject(0, 0, ganttChart);
         // Add a legend, showing whom is assigned to which tasks.
         // (If not using a grouping column, then adding a legend will be unnecessary.)
         canvas->SetFixedObject(0, 1,
            ganttChart->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph, false));
        @endcode*/
    class GanttChart final : public BarChart
        {
    public:
        /// @brief What to display on a task's bar.
        enum class TaskLabelDisplay
            {
            Resource,                   /*!< Display the name of the task.*/
            Description,                /*!< Display the description of the task.*/
            ResourceAndDescription,     /*!< Display the name description of the task.*/
            Days,                       /*!< Display the number of days in the task.*/
            ResourceAndDays,            /*!< Display the name of the task and number
                                             of days in it.*/
            DescriptionAndDays,         /*!< Display the description of the task and
                                             number of days in it.*/
            ResourceDescriptionAndDays, /*!< Display the name and description of the
                                             task and number of days in it.*/
            NoDisplay                   /*!< Don't display anything on the bar.*/
            };

        /// @brief Constructor.
        /// @param canvas The canvas that the chart is plotted on.
        /// @param colors The color scheme to apply to the boxes.
        ///  Leave as null to use the default theme.
        explicit GanttChart(Wisteria::Canvas* canvas,
            std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr);
        /** @brief Sets the data.
            @param data The data to use for the chart.
            @param interval The date interval to display across the axis.
             Note that this may affect the calculated starting and ending dates for the
             main axis. For example, using quarters will cause the dates to start and
             end at the beginning and end of a fiscal year.
            @param FYType The fiscal year type, which sets the fiscal year date range
             based on pre-defined types.
             This parameter is only relevant if @c interval is related to fiscal years.
            @param taskColumnName The column containing the task names.
            @param startDateColumnName The column containing the starting dates.
            @param endDateColumnName The column containing the ending date column.
            @param resourceColumnName The column containing whom the tasks are assigned to.
            @param descriptionColumnName The column containing descriptions of the tasks.
            @param completionColumnName The column containing the percentages of the
             tasks' completions (NaN will be treated as 0%).
            @param groupColumnName The grouping column to use.
             This will set the colors of the task's bars, based on their groups.
             Note that this can be the same column as the resource or task name columns.
            @throws std::runtime_error If any columns can't be found by name,
             throws an exception.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const DateInterval interval,
                     const FiscalYear FYType,
                     const wxString& taskColumnName,
                     const wxString& startDateColumnName,
                     const wxString& endDateColumnName,
                     std::optional<const wxString> resourceColumnName = std::nullopt,
                     std::optional<const wxString> descriptionColumnName = std::nullopt,
                     std::optional<const wxString> completionColumnName = std::nullopt,
                     std::optional<const wxString> groupColumnName = std::nullopt);

        /// @returns The date intervals as they are shown along the scaling axis.
        [[nodiscard]] DateInterval GetDateDisplayInterval() const noexcept
            { return m_dateDisplayInterval; }
        /// @returns The fiscal year type.
        [[nodiscard]] FiscalYear GetFiscalYearType() const noexcept
            { return m_fyType; }
        /// @returns Which information is being displayed across the tasks.
        [[nodiscard]] TaskLabelDisplay GetLabelDisplay() const noexcept
            { return m_labelDisplay; }
        /// @brief Sets which information to display across the tasks.
        /// @param labelDisplay The label display type.
        void SetLabelDisplay(const TaskLabelDisplay labelDisplay) noexcept
            { m_labelDisplay = labelDisplay; }
        /** @brief Builds and returns a legend using the current colors and labels.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.
             This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @param includeHeader `true` to show the group column name as the header.
            @returns The legend for the chart. Will return null if grouping was applied to the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendCanvasPlacementHint hint,
            const bool includeHeader);
    private:
        /// @brief Class to construct a task.
        /// @details This class has chainable calls which allow you to build it
        ///  inside of a call to GanttChart::AddTask().
        /// @note A task's color is controlled via the parent GanttChart's color scheme,
        ///  which is specified in its constructor.
        class TaskInfo
            {
            friend class GanttChart;
        public:
            /// @private
            TaskInfo() = default;
            /// @brief Constructor.
            /// @param name The name of the task.
            explicit TaskInfo(const wxString& name)
                { m_name = name; }
            /// @brief Sets who is carrying out the task.
            /// @param resource The resources assigned to the task.
            /// @returns A self reference.
            /// @note Adding newlines around the resource name will make it taller 
            ///  and hence will make the image next to it larger as well
            ///  (if you are displaying an image).
            /// @sa Image().
            TaskInfo& Resource(const wxString& resource)
                {
                m_resource = resource;
                return *this;
                }
            /// @brief Sets the name, which will appear on the Y axis.
            /// @param name The name of the task.
            /// @returns A self reference.
            TaskInfo& Name(const wxString& name)
                {
                m_name = name;
                return *this;
                }
            /// @brief Sets the description.
            /// @param description The description of the task.
            /// @returns A self reference.
            TaskInfo& Description(const wxString& description)
                {
                m_description = description;
                return *this;
                }
            /// @brief An image to be displayed next to the resource assigned to the task.
            /// @param img The image for the task.
            /// @returns A self reference.
            /// @note The image is scaled to the size of the resource name.
            ///  Hence, adding newlines around the resource name to make it taller
            ///  will also increase the size of the image.
            /// @sa Resource().
            TaskInfo& Image(const wxImage& img)
                {
                m_img = img;
                return *this;
                }
            /// @brief The start date of the task.
            /// @param start The start date of the task.
            ///  Leave as an invalid date to have the task start at the beginning of the timeline.
            ///  An arrow will be drawn to indicate that the task did not have a hard start date.
            /// @returns A self reference.
            TaskInfo& StartDate(const wxDateTime& start)
                {
                m_start = start;
                return *this;
                }
            /// @brief The end date of the task.
            /// @param end The end date of the task.
            ///  Leave as an invalid date have the task go to the end of the timeline.
            ///  An arrow will be drawn to indicate that the task does not have a hard end date.
            /// @returns A self reference.
            TaskInfo& EndDate(const wxDateTime& end)
                {
                m_end = end;
                return *this;
                }
            /// @brief How much of the task is already completed.
            /// @param percentFinished The percent (0-100) of how much is finished.
            /// @returns A self reference.
            TaskInfo& PercentFinished(const uint8_t percentFinished)
                {
                m_percentFinished = std::clamp<uint8_t>(percentFinished, 0, 100);
                return *this;
                }
            /// @brief Sets which information to display across the task.
            /// @param labelDisplay The label display type.
            /// @returns A self reference.
            TaskInfo& LabelDisplay(const TaskLabelDisplay labelDisplay) noexcept
                {
                m_labelDisplay = labelDisplay;
                return *this;
                }
            /// @brief The task's bar color.
            /// @param color The color of the bar.
            /// @returns A self reference.
            TaskInfo& Color(const wxColour& color) noexcept
                {
                m_color = color;
                return *this;
                }
        private:
            wxString m_resource;
            wxString m_name;
            wxString m_description;
            wxImage m_img;
            wxDateTime m_start;
            wxDateTime m_end;
            uint8_t m_percentFinished{ 0 };
            TaskLabelDisplay m_labelDisplay{ TaskLabelDisplay::Days };
            wxColour m_color{ *wxBLACK };
            };
        /** @brief Adds a task to the chart.
            @param taskInfo Information about the task.*/
        void AddTask(const TaskInfo& taskInfo)
            {
            m_tasks.emplace_back(taskInfo);
            Calculate();
            }
        void AddTask(TaskInfo&& taskInfo)
            {
            m_tasks.emplace_back(taskInfo);
            Calculate();
            }
        void Calculate();
        void RecalcSizes(wxDC& dc) final;
        
        /// @brief Get the color scheme used for the boxes.
        /// @returns The color scheme used for the boxes.
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }

        std::vector<TaskInfo> m_tasks;
        TaskLabelDisplay m_labelDisplay{ TaskLabelDisplay::Days };

        DateInterval m_dateDisplayInterval{ DateInterval::FiscalQuarterly };
        FiscalYear m_fyType{ FiscalYear::USBusiness };

        size_t m_maxDescriptionLength{ 75 };

        std::map<wxString, wxColour, Data::StringCmpNoCase> m_legendLines;
        wxString m_legendTitle;

        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme;
        };
    }

/** @}*/

#endif //__WISTERIA_GANTT_H__
