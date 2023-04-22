/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __SANKEY_DIAGRAM_H__
#define __SANKEY_DIAGRAM_H__

#include "graph2d.h"
#include "../util/frequencymap.h"

namespace Wisteria::Graphs
    {
    /** @brief Flow diagram, showing the flow of one series of groups into anther series of groups.

        @image html SankeyDiagram.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where two categorical columns represent the start and end
         groups and a continuous column can be an optional weight column.

         | From   | To   |
         | :--    | --:  |
         | Male   | Pass |
         | Male   | Pass |
         | Female | Fail |

         ...

         @par Missing Data:
          - Any missing data in an observation will result in listwise deletion.

         @par Example:
         @code
          // "this" will be a parent wxWidgets frame or dialog,
          // "canvas" is a scrolled window derived object
          // that will hold the plot
          auto canvas = new Wisteria::Canvas{ this };
          canvas->SetFixedObjectsGridSize(1, 1);

          SetTitle(_(L"Sankey Diagram"));

          auto sankeyData = std::make_shared<Data::Dataset>();
          try
              {
              sankeyData->ImportTSV(appDir + L"/datasets/historical/Titanic.csv",
                  ImportInfo().
                  CategoricalColumns({
                      { L"Sex", CategoricalImportMethod::ReadAsStrings },
                      { L"Embarked", CategoricalImportMethod::ReadAsStrings },
                      { L"Pclass", CategoricalImportMethod::ReadAsIntegers },
                      { L"Survived", CategoricalImportMethod::ReadAsIntegers }
                      }));
              }
          catch (const std::exception& err)
              {
              wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                           _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
              return;
              }

          auto sankey = std::make_shared<SankeyDiagram>(canvas);
          sankey->SetData(sankeyData, L"Sex", L"Survived", std::nullopt);
          sankey->SetCanvasMargins(5, 5, 5, 5);

          canvas->SetFixedObject(0, 0, sankey);

         @endcode
    */
    class SankeyDiagram : public Graph2D
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas to draw the diagram on.
            @param brushes The brush scheme to draw with.*/
        explicit SankeyDiagram(Canvas* canvas,
            std::shared_ptr<Brushes::Schemes::BrushScheme> brushes = nullptr) :
            Graph2D(canvas)
            {
            SetBrushScheme(brushes != nullptr ? brushes :
                std::make_shared<Brushes::Schemes::BrushScheme>(*Settings::GetDefaultColorScheme()));

            GetLeftYAxis().SetRange(0, 100, 0, 1, 10);
            GetLeftYAxis().Show(false);
            GetRightYAxis().Show(false);

            GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
            GetBottomXAxis().Show(false);
            GetTopXAxis().Show(false);
            }

        /** @brief Sets the data.
            @param data The data to use for the plot.
            @param fromColumnName The column containing the left-side groups.
            @param toColumnName The column containing the right-side groups that the
                left-side groups flow into.
            @param weightColumnName The (optional) column containing the multiplier value for each row.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                 The exception's @c what() message is UTF-8 encoded, so pass it to
                 @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& fromColumnName, const wxString& toColumnName,
                     const std::optional<wxString>& weightColumnName);

        /// @private
        [[deprecated("Sankey diagram does not support legends.")]]
        [[nodiscard]]
        std::shared_ptr<GraphItems::Label> CreateLegend(
            [[maybe_unused]] const LegendOptions& options) final
            {
            wxFAIL_MSG(L"Sankey diagram does not support legends.");
            return nullptr;
            }
    private:
        class SankeyGroup
            {
        public:
            using DownStreamGroups = frequency_set<wxString, Data::wxStringLessNoCase, double>;

            explicit SankeyGroup(wxString label) :
                m_label(std::move(label))
                {}
            SankeyGroup(wxString label, const double freq,
                        const DownStreamGroups& downStreamGroups) :
                m_label(std::move(label)), m_frequency(freq), m_downStreamGroups(downStreamGroups)
                {}
            wxString m_label;
            double m_frequency{ 0 };
            double m_percentOfColumn{ 0 };
            double m_currentYAxisPosition{ 0 };
            double m_yAxisTopPosition{ 0 };
            double m_yAxisBottomPosition{ 0 };
            double m_yAxisWidth{ 0 };
            double m_xAxisLeft{ 0 };
            double m_xAxisRight{ 0 };
            DownStreamGroups m_downStreamGroups;
            void OffsetY(const double offset) noexcept
                {
                m_yAxisTopPosition += offset;
                m_yAxisBottomPosition += offset;
                m_currentYAxisPosition += offset;
                }
            [[nodiscard]]
            bool operator<(const SankeyGroup& that) const
                { return m_label.CmpNoCase(that.m_label) < 0; }
            [[nodiscard]]
            bool operator==(const SankeyGroup& that) const
                { return m_label.CmpNoCase(that.m_label) == 0; }
            };
        using SankeyColumn = std::vector<SankeyGroup>;

        std::vector<SankeyColumn> m_sankeyColumns;

        /// @brief Recalculates the size of embedded objects on the plot.
        void RecalcSizes(wxDC& dc) final;
        };
    }

/** @}*/

#endif // __SANKEY_DIAGRAM_H__
