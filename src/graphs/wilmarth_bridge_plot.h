/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_WILMARTH_BRIDGE_PLOT_H
#define WISTERIA_WILMARTH_BRIDGE_PLOT_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief A grid of observations that fade out of a study over a series of periods,
            drawn as a character-aligned table whose tapering silhouette traces the
            survival curve.
        @details Each column is one observation, drawn in the same character position for
            every period it is under observation. Each row is a period. An observation's
            label stops appearing once it exits the study, so the surviving column count
            for each row is the number at risk at that period, and the shrinking silhouette
            of the grid is the survival curve.
        @image html WilmarthBridgePlot.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset with one categorical or continuous column for the
         observations' labels (a continuous column works for discrete numeric IDs) and one
         continuous or date column for when each observation exited the study. Optionally, a
         second continuous or date column (of the same type as the exit column) can specify
         when each observation entered, and a column can flag whether an observation's exit
         was an event or a right-censoring.\n
         \n
         The status and intermediate event flags each accept either a continuous 0/1 column
         or a categorical column of 0/1 codes.

         | Letter | Entered | Faded | Status |
         | :--    | --:     | --:   | --:    |
         | F      | 1963    | 1967  |        |
         | O      | 1963    | 1963  |        |
         | R      | 1963    | 1971  | 0      |

        @par Missing Data:
         - A missing exit value results in pairwise deletion: that observation is not drawn
           and is excluded from the survival statistics, although its column position is
           still reserved (a fully missing-data row therefore renders as a blank column,
           which is a convenient way to separate groups of observations visually).
         - A missing entry value defaults to the first period on the chart.
         - A missing label is drawn and counted as an observation, just with blank cells.
         - A missing status value is treated as an event, not a censoring.
         - A missing entry value on the row where an intermediate event occurred leaves that
           observation's transition time unknown. It will be drawn in the normal label color
           for its whole span instead of switching to GetIntermediateEventColor() partway
           through.

        @par Censoring:
         The exit column always holds the last period an observation was under
         observation, whether that period was when the event happened or merely the last
         time the observation was known to still be ongoing. The status column says which:
         a value of @c 1 means the event occurred, @c 0 means the observation was
         right-censored (it was still ongoing when observation stopped). This is the
         polarity used by R's `Surv(time, event)`; a dataset using a *censoring* indicator
         (the opposite polarity) needs its status values inverted first.\n
         \n
         A censored observation's cells stop at its exit period, same as any other
         observation, followed by an arrow icon indicating that its outcome past that
         point is unknown. It is never drawn past its exit period, and it is never dropped
         from the survival statistics; dropping it would bias the estimate, since it is
         known to have lasted at least that long.

        @par Intermediate Events:
         Some studies record a time-dependent event partway through an observation's
         timeline (e.g., a heart transplant received while awaiting one). Such an
         observation is split across two rows: one spanning entry to the intermediate
         event, the other spanning the intermediate event to the final exit. Passing an
         intermediate event column to SetData() tells the plot to combine the two rows
         into a single observation. Then it will color its cells from the intermediate event
         onward with GetIntermediateEventColor(), leaving the earlier cells in the
         normal label color.\n
         \n
         The intermediate event column holds @c 1 for a row occurring after the event and
         @c 0 (or missing) beforehand. Rows sharing the same value in the label
         column are combined into one observation, acting as the observation's ID.
         Rows belonging to the same observation must appear in chronological order.
         The observation's overall entry is taken from its first row and its exit and
         status from its last.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };

         auto inscriptionData = std::make_shared<Wisteria::Data::Dataset>();
         try
            {
            inscriptionData->ImportCSV(L"/home/kdaly/Documents/Wilmarth Bridge.csv",
                ImportInfo().
                ContinuousColumns({ L"Entered", L"Faded" }).
                CategoricalColumns({ { L"Letter" } }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK | wxICON_ERROR| wxCENTRE);
            return;
            }

         auto bridgePlot = std::make_shared<WilmarthBridgePlot>(canvas);
         bridgePlot->SetData(inscriptionData, L"Letter", L"Faded", L"Entered");

         bridgePlot->SetCanvasMargins(5, 5, 5, 5);
         canvas->SetFixedObject(0, 0, bridgePlot);
        @endcode

        @par Citation:
         Susan Wilmarth's inscription to the sculptor Christopher Wilmarth, chalked on the
         Brooklyn Bridge footwalk railing and observed annually from 1963 until it faded
         away entirely by October 1971.\n
         \n
         Tufte, E. R. (1997). <i>Visual Explanations: Images and Quantities, Evidence and
         Narrative</i> (p. 84). Graphics Press.*/
    // clang-format on

    class WilmarthBridgePlot final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(WilmarthBridgePlot);
        WilmarthBridgePlot() = default;

      public:
        /// @brief How an observation's label ink weakens over its lifetime.
        enum class FadeEffect
            {
            None,              /*!< Constant ink, as in the printed plate.*/
            RemainingLifetime, /*!< An observation is palest during the period just before
                                    it exits the study.*/
            ElapsedTime        /*!< Every row is fainter than the one above it, so the whole
                                    grid washes out from top to bottom.*/
            };

        /// @brief What survival statistics to display alongside the grid.
        enum class SurvivalDisplay
            {
            None,            /*!< Do not display any survival statistics.*/
            AtRiskCount,     /*!< The number of observations still at risk at each period.*/
            SurvivalPercent, /*!< The Kaplan-Meier survival estimate at each period.*/
            Both             /*!< Both the at-risk count and the survival percent.*/
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.*/
        explicit WilmarthBridgePlot(Canvas* canvas);

        /** @brief Sets the data for the plot.
            @param data The data to use.
            @param labelColumnName The categorical or continuous column containing the
                observations' labels/IDs (e.g., the letters of the inscription, or a column of
                discrete numeric IDs).
            @param exitColumnName The continuous or date column containing when each
                observation was last under observation.
            @param entryColumnName The continuous or date column (matching the type of
                @p exitColumnName) containing when each observation entered the study.\n
                If not provided, every observation is assumed to enter at the first period.
            @param statusColumnName The continuous or categorical column containing each
                observation's status: @c 1 for an event, @c 0 for a right-censored
                observation.\n
                If not provided, every observation is assumed to have had an event.
            @param intermediateEventColumnName The continuous or categorical column flagging
                whether a row occurred after a time-dependent intermediate event (@c 1) or
                before it (@c 0 or missing).\n
                If provided, contiguous rows sharing the same label/ID are combined into a single
                observation, and its cells from the intermediate event onward are colored
                with GetIntermediateEventColor(). Not provided by default, so every row is
                treated as its own observation if not provided.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, or if the
                exit and entry columns don't resolve to the same column type, throws an
                exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& labelColumnName, const wxString& exitColumnName,
                     const std::optional<wxString>& entryColumnName = std::nullopt,
                     const std::optional<wxString>& statusColumnName = std::nullopt,
                     const std::optional<wxString>& intermediateEventColumnName = std::nullopt);

        /// @returns How an observation's label ink weakens over its lifetime.
        [[nodiscard]]
        FadeEffect GetFadeEffect() const noexcept
            {
            return m_fadeEffect;
            }

        /// @brief Sets how an observation's label ink weakens over its lifetime.
        /// @param effect The fade effect to use (default is @c FadeEffect::None,
        ///     matching the constant ink of the printed plate).
        void SetFadeEffect(const FadeEffect effect) noexcept { m_fadeEffect = effect; }

        /// @returns What survival statistics are being displayed alongside the grid.
        [[nodiscard]]
        SurvivalDisplay GetSurvivalDisplay() const noexcept
            {
            return m_survivalDisplay;
            }

        /// @brief Sets what survival statistics to display alongside the grid.
        /// @param display The statistics to display (default is @c SurvivalDisplay::None).
        void SetSurvivalDisplay(const SurvivalDisplay display)
            {
            m_survivalDisplay = display;
            if (GetDataset() != nullptr)
                {
                UpdateAxes();
                }
            }

        /// @returns The name of the categorical column that the labels came from.
        [[nodiscard]]
        const wxString& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @returns The name of the column that the exit periods came from.
        [[nodiscard]]
        const wxString& GetExitColumnName() const noexcept
            {
            return m_exitColumnName;
            }

        /// @returns The name of the column that the entry periods came from, or an
        ///     empty string if one wasn't provided.
        [[nodiscard]]
        const wxString& GetEntryColumnName() const noexcept
            {
            return m_entryColumnName;
            }

        /// @returns The name of the column that the event/censoring status came from,
        ///     or an empty string if one wasn't provided.
        [[nodiscard]]
        const wxString& GetStatusColumnName() const noexcept
            {
            return m_statusColumnName;
            }

        /// @returns The name of the column that the intermediate event flag came from,
        ///     or an empty string if one wasn't provided.
        [[nodiscard]]
        const wxString& GetIntermediateEventColumnName() const noexcept
            {
            return m_intermediateEventColumnName;
            }

        /// @returns The color used for an observation's cells from its intermediate
        ///     event onward.
        [[nodiscard]]
        const wxColour& GetIntermediateEventColor() const noexcept
            {
            return m_intermediateEventColor;
            }

        /// @brief Sets the color used for an observation's cells from its intermediate
        ///     event onward.
        /// @param color The color to use. Ignored if invalid.
        void SetIntermediateEventColor(const wxColour& color)
            {
            if (color.IsOk())
                {
                m_intermediateEventColor = color;
                }
            }

        /// @returns @c true if censored observations are marked with an arrow.
        [[nodiscard]]
        bool IsShowingCensoredMarkers() const noexcept
            {
            return m_showCensoredMarkers;
            }

        /// @brief Whether to mark censored observations with an arrow past their
        ///     final cell.
        /// @param show @c true to show the markers (the default).
        void ShowCensoredMarkers(const bool show) noexcept { m_showCensoredMarkers = show; }

        /// @returns The label for the terminal row appended after the last observed
        ///     period, or an empty string if no terminal row is shown (the default).
        [[nodiscard]]
        const wxString& GetTerminalRowLabel() const noexcept
            {
            return m_terminalRowLabel;
            }

        /// @brief Appends an extra row after the last observed period, labeled with
        ///     @p label, to confirm the study's actual endpoint. For example, the printed
        ///     plate's "Oct 1971" row, added after the last annual observation to
        ///     confirm that the final letters had faded away by then.
        /// @param label The label for the terminal row. An empty label removes it
        ///     (the default).
        /// @note The row is only drawn if every observation had an event (i.e., none
        ///     are right-censored). A censored observation's status past its exit
        ///     period is unknown, so the terminal row would be asserting something
        ///     that was never actually observed.
        void ShowTerminalRow(const wxString& label)
            {
            m_terminalRowLabel = label;
            if (GetDataset() != nullptr)
                {
                UpdateAxes();
                }
            }

        /// @deprecated
        [[deprecated("Wilmarth bridge plots do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            return nullptr;
            }

      private:
        /// @brief A single observation, resolved from the dataset into a common
        ///     numeric time scale (continuous values as-is, dates as their modified
        ///     Julian day number).
        struct Observation
            {
            wxString m_label;
            double m_entered{ 0 };
            double m_exit{ 0 };
            bool m_censored{ false };
            size_t m_datasetRow{ 0 };
            // the period at which this observation's intermediate event occurred
            std::optional<double> m_intermediateEventPeriod;
            };

        void SetAutoAccessibilityAttributes() final;
        void RecalcSizes(wxDC& dc) final;

        /// @returns The distinct period values across all observations, ascending.
        [[nodiscard]]
        std::vector<double> BuildPeriods() const;

        /// @brief Configures the bottom, left, and right axes from the current data
        ///     and display options.
        /// @note Must run before Graph2D::RecalcSizes() assigns physical coordinates
        ///     to the axes, so this is called from SetData() and from the setters that
        ///     affect axis content, not from RecalcSizes().
        void UpdateAxes();

        /// @returns The number of observations at risk just before @p period (i.e.,
        ///     entered at or before it and not yet exited).
        [[nodiscard]]
        size_t AtRiskCount(double period) const;

        /// @returns The number of events (not censorings) occurring at exactly @p period.
        [[nodiscard]]
        size_t EventCount(double period) const;

        /// @returns The Kaplan-Meier survival estimate through @p period, inclusive.
        [[nodiscard]]
        double SurvivalProbability(double period) const;

        /// @returns @p period formatted for display, as a date if the exit column was
        ///     a date column, otherwise as a plain number.
        [[nodiscard]]
        wxString FormatPeriod(double period) const;

        /// @returns The at-risk count and/or survival percent for @p period, formatted
        ///     according to the current survival display setting.
        [[nodiscard]]
        wxString FormatSurvivalStatText(double period) const;

        /// @returns @p baseColor, weakened according to the current fade effect for
        ///     @p obs at @p period.
        [[nodiscard]]
        wxColour FadeColor(const wxColour& baseColor, const Observation& obs, double period) const;

        std::vector<Observation> m_observations;
        std::vector<double> m_periods;

        // whether the exit/entry columns were resolved as dates (for label formatting)
        bool m_usingDateColumns{ false };

        wxString m_labelColumnName;
        wxString m_exitColumnName;
        wxString m_entryColumnName;
        wxString m_statusColumnName;
        wxString m_intermediateEventColumnName;

        FadeEffect m_fadeEffect{ FadeEffect::None };
        SurvivalDisplay m_survivalDisplay{ SurvivalDisplay::None };
        bool m_showCensoredMarkers{ true };
        wxString m_terminalRowLabel;
        wxColour m_intermediateEventColor{ Colors::ColorBrewer::GetColor(
            Colors::Color::SeaGreen) };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WILMARTH_BRIDGE_PLOT_H
