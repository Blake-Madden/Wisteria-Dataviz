/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_DATA_GUIDE_CTRL_H
#define WISTERIA_DATA_GUIDE_CTRL_H

#include "../../data/dataset.h"
#include <vector>
#include <wx/webview.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief How a graph expects its dataset to be laid out.
    enum class DataGuideShape
        {
        Unspecified, /*!< No layout is shown.*/
        Long,        /*!< Each row is one observation, with a column per variable.*/
        Wide         /*!< Each row is one subject, with repeated measures spread across columns.*/
        };

    /// @brief Whether a variable must be supplied.
    enum class DataGuideRequirement
        {
        Required,   /*!< Always needed.*/
        Optional,   /*!< Never needed.*/
        Conditional /*!< Needed only in certain situations (see DataGuideParameter::m_condition).*/
        };

    /// @brief A value with a special meaning in a variable (e.g., a 1 meaning "event").
    struct DataGuideValue
        {
        /// @brief The value as it appears in the data.
        wxString m_value;
        /// @brief What the value means.
        wxString m_meaning;
        };

    /// @brief A variable that the graph reads (as listed in the dialog for selecting variables).
    struct DataGuideParameter
        {
        /// @brief The label of the variable in the variable selection dialog.
        wxString m_name;
        /// @brief The types of data the variable accepts.
        std::vector<Data::Dataset::ColumnImportType> m_types;
        /// @brief Whether the graph requires this variable.
        DataGuideRequirement m_requirement{ DataGuideRequirement::Required };
        /// @brief An explanation of how the graph uses the variable.
        wxString m_description;
        /// @brief The coded values the variable expects, if any.
        /// @details The "Expected Values" column of the guide is only shown when a variable has
        /// some.
        std::vector<DataGuideValue> m_values;
        /// @brief @c true if the variable accepts several columns, @c false if it takes one.
        bool m_multiple{ false };
        /// @brief When the variable is needed (shown for DataGuideRequirement::Conditional).
        wxString m_condition;
        /// @brief The heading to list the variable under (e.g., "Face features").
        /// @details A heading is shown whenever this changes from the previous variable,
        ///     so variables of the same group should be adjacent.
        wxString m_group;
        };

    /// @brief A column of a sample dataset.
    struct DataGuideColumn
        {
        /// @brief The column name shown in the sample table.
        wxString m_name;
        /// @brief The name of the DataGuideParameter that the column is selected as.
        wxString m_parameter;
        };

    /// @brief A sample dataset.
    struct DataGuideExample
        {
        /// @brief An optional heading describing the scenario (e.g., "Patient follow-up").
        wxString m_title;
        /// @brief The columns of the sample dataset.
        std::vector<DataGuideColumn> m_columns;
        /// @brief The rows of the sample dataset (one cell per column).
        std::vector<std::vector<wxString>> m_rows;
        };

    /// @brief The content shown by DataGuideCtrl.
    struct DataGuide
        {
        /// @brief The heading of the guide.
        wxString m_title;
        /// @brief The resource name of the graph's icon, shown beside the title.
        wxString m_iconName;
        /// @brief A brief description of what the graph plots and its purpose,
        ///     shown as the subtitle of the banner.
        wxString m_summary;
        /// @brief A description of the shape of dataset the graph expects
        ///     (e.g., what one row represents).
        wxString m_requirements;
        /// @brief The layout of dataset the graph expects. A graphic and label explaining the
        ///     layout are shown with the data requirements.
        DataGuideShape m_shape{ DataGuideShape::Unspecified };
        /// @brief Every variable the graph can use.
        std::vector<DataGuideParameter> m_parameters;
        /// @brief Things that must be in place that aren't columns
        ///     (e.g., labels assigned to the columns after importing).
        std::vector<wxString> m_prerequisites;
        /// @brief The sample datasets.
        std::vector<DataGuideExample> m_examples;
        /// @brief What happens when a variable's value is missing, one statement per variable.
        std::vector<wxString> m_missingData;
        /// @brief Additional remarks.
        std::vector<wxString> m_notes;

        /// @returns @c true if there is nothing to show.
        [[nodiscard]]
        bool IsEmpty() const noexcept
            {
            return m_title.empty() && m_summary.empty() && m_requirements.empty() &&
                   m_parameters.empty() && m_examples.empty();
            }
        };

    /** @brief Panel that presents a DataGuide as a styled page.
        @details The page follows the system colors and is rebuilt when they change.*/
    class DataGuideCtrl final : public wxPanel
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param id The window ID.
            @param pos The position.
            @param size The size.*/
        explicit DataGuideCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize);

        /// @private
        DataGuideCtrl(const DataGuideCtrl&) = delete;
        /// @private
        DataGuideCtrl& operator=(const DataGuideCtrl&) = delete;

        /** @brief Sets the content to display.
            @param guide The content to show.*/
        void SetGuide(const DataGuide& guide);

      private:
        [[nodiscard]]
        wxString BuildHtml() const;
        [[nodiscard]]
        static wxString EscapeForHtml(const wxString& text);
        [[nodiscard]]
        static wxString GetTypeName(Data::Dataset::ColumnImportType type);
        [[nodiscard]]
        static wxString GetSvgDataUri(const wxString& svgName, const wxSize& size);
        [[nodiscard]]
        static wxString GetTypeIconUri(Data::Dataset::ColumnImportType type);
        [[nodiscard]]
        static wxString BuildTypeIconHtml(Data::Dataset::ColumnImportType type);
        [[nodiscard]]
        static wxString GetAccentStyle(size_t roleIndex);
        // the page's CSS, split into sections to keep each string literal small
        [[nodiscard]]
        static wxString GetBaseStyles();
        [[nodiscard]]
        static wxString GetHeroStyles();
        [[nodiscard]]
        static wxString GetTableStyles();
        [[nodiscard]]
        static wxString GetChipStyles();
        [[nodiscard]]
        static wxString GetSectionStyles();
        /// @brief Builds a small table sketch (long is tall and narrow, wide is short and broad)
        ///     plus a label for a dataset layout.
        /// @param shape The layout to show.
        /// @returns The HTML, or an empty string if @p shape is unspecified.
        [[nodiscard]]
        static wxString BuildShapeHtml(DataGuideShape shape);
        [[nodiscard]]
        static wxString BuildBulletCard(const std::vector<wxString>& items,
                                        const wxString& cardClass);
        [[nodiscard]]
        static wxString BuildSectionTitle(const wxString& title, const wxString& iconMarkup,
                                          const wxString& badgeClass = wxString{});
        void RefreshPage();
        void OnSysColourChanged(wxSysColourChangedEvent& event);
        void OnNavigating(wxWebViewEvent& event);

        wxWebView* m_webView{ nullptr };
        DataGuide m_guide;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_DATA_GUIDE_CTRL_H
