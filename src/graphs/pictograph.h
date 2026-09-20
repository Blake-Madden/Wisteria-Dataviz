/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PICTOGRAPH_H
#define WISTERIA_PICTOGRAPH_H

#include "../base/shapes.h"
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A pictograph, where a single vertical or horizontal bar of shapes
            is drawn and each shape's area is proportional to its value.
        @details The same icon is used for every observation. The smallest value
            is drawn with the smallest icon, and the remaining icons are scaled up so that
            the ratio of their areas matches the ratio of their values.
            No icon's side is drawn more than 100 times the smallest icon's side, so
            values beyond a 10,000:1 ratio to the smallest value are no longer
            proportional.

            Each icon has its label followed by its value in parentheses. Vertical bars
            place these labels to the right of the icons, and horizontal bars place them
            on a baseline just below the largest icon.

            Labels are drawn at the default font size. If horizontal labels overlap, then
            they alternate between a baseline just below the largest icon and one just above it
            (starting below for the first icon).

        @par %Data:
            This plot accepts a Data::Dataset with a continuous column (the values)
            and a categorical or ID column (the labels).

        @par Observation Limits:
            This chart is limited to 10 observations. Beyond this, the smallest
            icons become too small to display their values.

        @par Missing Data:
            Observations with a missing or non-positive value are skipped, as an
            area can't be drawn for them.

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 1);

         auto moneyData = std::make_shared<Data::Dataset>();
         moneyData->ImportCSV(L"deposits.csv",
            ImportInfo().
            ContinuousColumns({ L"Amount" }).
            CategoricalColumns({ { L"Year", CategoricalImportMethod::ImportAsStrings } }));

         auto pictograph = std::make_shared<Pictograph>(
            canvas, Icons::IconShape::PropertyBag, Orientation::Vertical);
         pictograph->SetData(moneyData, L"Amount", L"Year");

         canvas->SetFixedObject(0, 0, pictograph);
        @endcode*/
    class Pictograph final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(Pictograph);
        Pictograph() = default;

      public:
        /// @brief Maximum number of icons that can be displayed effectively.
        constexpr static size_t MAX_ICONS = 10;
        /// @brief The largest size (in DIPs) that the smallest icon's sides can grow to.
        /// @details The icons are scaled (retaining their proportions) to fill the plot area,
        ///     between @c HARD_MIN_ICON_SIZE_DIPS and this size.
        constexpr static int MAX_ICON_SIZE_DIPS = 128;
        /// @brief The smallest size (in DIPs) that the smallest icon's sides can be shrunk to.
        /// @details If the icons still don't fit at this size, then they are drawn at this size
        ///     and overlapped by an equal amount.
        constexpr static int HARD_MIN_ICON_SIZE_DIPS = 1;

        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param shape The shape used for every icon.
            @param orientation Whether the icons are stacked vertically (top to bottom)
                or laid out horizontally (left to right).\n
                @c Orientation::Both is treated as @c Orientation::Vertical.*/
        explicit Pictograph(Canvas* canvas, Icons::IconShape shape,
                            Orientation orientation = Orientation::Vertical);

        /** @brief Sets the data for the pictograph.
            @param data The data to use.
            @param valueColumn The continuous column whose values control the icons' areas.
            @param labelColumn The categorical column (or the ID column) whose values
                are drawn beneath the icons.
            @warning If the dataset contains more than @c MAX_ICONS observations,
                only the first @c MAX_ICONS will be displayed and a warning will be logged.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset
                to re-plot the data.
            @throws std::runtime_error If either column can't be found by name.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data, const wxString& valueColumn,
                     const wxString& labelColumn);

        /// @returns The name of the value column that was last passed to SetData().
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns The name of the label column that was last passed to SetData().
        [[nodiscard]]
        const wxString& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @name Appearance Functions
        /// @brief Functions relating to the visual appearance of the icons.
        /// @{

        /// @returns The brush that the icons are filled with by default.
        [[nodiscard]]
        static wxBrush GetDefaultIconBrush()
            {
            return wxBrush{ wxColour{ 70, 130, 180 } };
            }

        /// @returns The pen that the icons are outlined with by default.
        [[nodiscard]]
        static wxPen GetDefaultIconPen()
            {
            return wxPen{ *wxBLACK };
            }

        /// @returns The shape used for every icon.
        [[nodiscard]]
        Icons::IconShape GetShape() const noexcept
            {
            return m_shape;
            }

        /// @returns The orientation of the icons.
        [[nodiscard]]
        Orientation GetOrientation() const noexcept
            {
            return m_orientation;
            }

        /// @returns The brush used to fill the icons.
        [[nodiscard]]
        const wxBrush& GetIconBrush() const noexcept
            {
            return m_iconBrush;
            }

        /// @brief Sets the brush used to fill the icons.
        /// @param brush The brush to use.
        void SetIconBrush(const wxBrush& brush)
            {
            if (brush.IsOk())
                {
                m_iconBrush = brush;
                }
            }

        /// @returns The pen used to outline the icons.
        [[nodiscard]]
        const wxPen& GetIconPen() const noexcept
            {
            return m_iconPen;
            }

        /// @brief Sets the pen used to outline the icons.
        /// @param pen The pen to use.
        void SetIconPen(const wxPen& pen)
            {
            if (pen.IsOk())
                {
                m_iconPen = pen;
                }
            }

        /// @returns How the values are formatted when displayed in the labels.
        [[nodiscard]]
        NumberDisplay GetValueFormat() const noexcept
            {
            return m_valueFormat;
            }

        /// @brief Sets how the values are formatted when displayed in the labels.
        /// @param format The format to display values with (default is @c NumberDisplay::Value).
        void SetValueFormat(const NumberDisplay format) noexcept { m_valueFormat = format; }

        /// @}

        /** @brief Pictographs label their icons directly, so no legend is needed.
            @param options This parameter is ignored.
            @returns @c nullptr.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

      private:
        /// @brief A single observation.
        struct IconInfo
            {
            double m_value{ 0.0 };
            wxString m_label;
            };

        void RecalcSizes(wxDC& dc) final;

        void SetAutoAccessibilityAttributes() final;

        /// @returns A value, formatted according to @c GetValueFormat().
        /// @param value The value to format.
        /// @param thousandsSeparator @c false to leave out thousands separators.
        [[nodiscard]]
        wxString FormatValue(double value, bool thousandsSeparator = true) const;

        std::vector<IconInfo> m_icons;
        wxString m_valueColumnName;
        wxString m_labelColumnName;
        Icons::IconShape m_shape{ Icons::IconShape::Square };
        Orientation m_orientation{ Orientation::Vertical };
        wxBrush m_iconBrush{ GetDefaultIconBrush() };
        wxPen m_iconPen{ GetDefaultIconPen() };
        NumberDisplay m_valueFormat{ NumberDisplay::Value };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_PICTOGRAPH_H
