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
#include "colorbrewer.h"

namespace Wisteria::Graphs
    {
    /** @brief A chart which shows the progress of events (e.g., tasks) along a timeline.
        @details These are useful for project management.
        @image html GanttChart.svg width=90%

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 1);

         // just use the current year to keep this example simple
         const auto thisYear = wxDateTime::Now().GetYear();

         auto ganttChart = std::make_shared<GanttChart>(canvas);
         ganttChart->SetFiscalYearType(GanttChart::FiscalYear::Business);

         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Acquisition")).
            Resource(_(L"Management")).
            StartDate(wxDateTime(1, wxDateTime::Jul, thisYear)).
            EndDate(wxDateTime(31, wxDateTime::Aug, thisYear)).
            PercentFinished(100) );
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Develop Product")).
            Resource(_(L"Development")).
            StartDate(wxDateTime(1, wxDateTime::Sep, thisYear)).
            EndDate(wxDateTime(25, wxDateTime::Dec, thisYear)).
            PercentFinished(50));
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Testing")).
            Resource(_(L"QA")).
            StartDate(wxDateTime(15, wxDateTime::Oct, thisYear)).
            EndDate(wxDateTime(25, wxDateTime::Dec, thisYear)));
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Update Help")).
            Resource(_(L"Tech Writers")).
            StartDate(wxDateTime(15, wxDateTime::Nov, thisYear)).
            EndDate(wxDateTime(25, wxDateTime::Dec, thisYear)));
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Terminations")).
            Resource(_(L"Human Resources")).
            Description(_(L"Round 1: just QA and tech writers")).
            StartDate(wxDateTime(26, wxDateTime::Dec, thisYear)).
            EndDate(wxDateTime(15, wxDateTime::Jan, thisYear+1)));
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Product Fixes")).
            Resource(_(L"Development")).
            Description(_(L"Need two weeks to sell this to get a quarterly bump")).
            StartDate(wxDateTime(26, wxDateTime::Dec, thisYear)).
            EndDate(wxDateTime(15, wxDateTime::Mar, thisYear+1)));
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Terminations")).
            Resource(_(L"Human Resources")).
            Description(_(L"Round 2: remaining developers")).
            StartDate(wxDateTime(16, wxDateTime::Mar, thisYear+1)).
            EndDate(wxDateTime(31, wxDateTime::Mar, thisYear+1)));
         ganttChart->AddTask(GanttChart::TaskInfo(_(L"Profit")).
            Resource(_(L"Management")).
            StartDate(wxDateTime(25, wxDateTime::Dec, thisYear)));

         ganttChart->AddReferenceLine(ReferenceLine(AxisType::BottomXAxis,
            ganttChart->GetScalingAxis().GetPointFromDate(
                wxDateTime(25, wxDateTime::Dec, thisYear)),
            _(L"First Release")) );
        ganttChart->AddReferenceLine(ReferenceLine(AxisType::BottomXAxis,
            ganttChart->GetScalingAxis().GetPointFromDate(
                wxDateTime(15, wxDateTime::Mar, thisYear + 1)),
            _(L"Update Release")));

         ganttChart->SetCanvasMargins(5, 5, 5, 5);
         canvas->SetFixedObject(0, 0, ganttChart);
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
            ResourceAndDays,            /*!< Display the name of the task and number of days in it.*/
            DescriptionAndDays,         /*!< Display the description of the task and number of days in it.*/
            ResourceDescriptionAndDays, /*!< Display the name and description of the task and number of days in it.*/
            NoDisplay                   /*!< Don't display anything on the bar.*/
            };

        /// @brief Class to construct a task.
        /// @details This class has chainable calls which allow you to build it inside of a call to GanttChart::AddTask().
        /// @note A task's color is controlled via the parent GanttChart's color scheme, which is specified in its constructor.
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
            /// @note Adding newlines around the resource name will make it taller and hence will make the image
            ///  next to it larger as well (if you are displaying an image).
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
            /// @note The image is scaled to the size of the resource name. Hence, adding newlines
            ///  around the resource name to make it taller will also increase the size of the image.
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
            /// @brief What to display on the task's label.
            /// @param labelDisplay The label display type.
            /// @returns A self reference.
            TaskInfo& LabelDisplay(const TaskLabelDisplay labelDisplay) noexcept
                {
                m_labelDisplay = labelDisplay;
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
            };

        /// @brief Constructor.
        /// @param canvas The canvas that the chart is plotted on.
        /// @param colors The color scheme to apply to the boxes.
        ///  Leave as null to use an Earth tones theme.
        explicit GanttChart(Wisteria::Canvas* canvas,
            std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr);
        /** @brief Adds a task to the chart.
            @param taskInfo Information about the task.*/
        void AddTask(const TaskInfo& taskInfo)
            {
            m_tasks.emplace_back(taskInfo);
            Calculate();
            }

        /// @brief Sets the fiscal year date range, based on pre-defined types.
        /// @details Use SetFiscalYearStart() to set a specific start date.
        /// @param FY The fiscal year type.
        /// @sa SetDateDisplayInterval().
        void SetFiscalYearType(const FiscalYear FY)
            { m_fyType = FY; }
        /// @returns The fiscal year type.
        [[nodiscard]] FiscalYear GetFiscalYearType() const noexcept
            { return m_fyType; }
        /** @brief Sets the date intervals that are shown along the scaling axis.
            @details This also affects the starting and ending points of the dates.
             For example, using quarters will cause the dates to start and end
             at the beginning and end of a fiscal year.
            @param interval The date interval to use.*/
        void SetDateDisplayInterval(const DateInterval interval) noexcept
            { m_dateDisplayInterval = interval; }
        /// @returns The date intervals as they are shown along the scaling axis.
        [[nodiscard]] DateInterval GetDateDisplayInterval() const noexcept
            { return m_dateDisplayInterval; }

        /// @private
        void AddTask(TaskInfo&& taskInfo)
            {
            m_tasks.emplace_back(taskInfo);
            Calculate();
            }
    private:
        void Calculate();
        void RecalcSizes() final;
        /// @brief Get the color scheme used for the boxes.
        /// @returns The color scheme used for the boxes.
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }

        std::vector<TaskInfo> m_tasks;

        DateInterval m_dateDisplayInterval{ DateInterval::FiscalQuarterly };
        FiscalYear m_fyType{ FiscalYear::USBusiness };

        size_t m_maxDescriptionLength{ 75 };

        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme;
        };
    }

/** @}*/

#endif //__WISTERIA_GANTT_H__
