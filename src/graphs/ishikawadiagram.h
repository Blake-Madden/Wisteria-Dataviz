/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef ISHIKAWA_DIAGRAM_H
#define ISHIKAWA_DIAGRAM_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief Cause-and-effect (Ishikawa or fishbone) diagram.

         A spine leads to the effect (the problem being analyzed), with branches
         (categories of causes) angled off of it, alternating above and below.
         Each branch lists its causes, and each cause can have sub-causes.

        @par Citation:
         Ishikawa, Kaoru. <i>Guide to Quality Control</i>. Tokyo: Asian Productivity
         Organization, 1976.

        @par %Data:
         This plot accepts a Data::Dataset with one categorical column per branch.
         Each cell is a cause, and blank cells are ignored. A cause can also have
         its own column of sub-causes.

         | Man      | Machine | Material | Method    | Measure       | Environment |
         | :--      | :--     | :--      | :--       | :--           | :--         |
         | Training | Speed   | Alloy    | Procedure | Micrometers   | Humidity    |
         | Fatigue  | Age     | Supplier |           | Gauge wear    |             |

         @par Missing Data:
          - Blank causes and sub-causes are skipped.
          - Sub-causes of a cause that isn't on its branch are skipped.
          - A cause that is listed more than once on a branch is shown once.

         @par Example:
         @code
          auto ishikawa = std::make_shared<IshikawaDiagram>(canvas);
          ishikawa->SetData(causeData, _(L"Surface flaws"),
              { { _(L"Personnel"), L"Man", { { L"Training", L"Training Details" } } },
                { _(L"Machines"), L"Machine" } });
          canvas->SetFixedObject(0, 0, ishikawa);
         @endcode
    */
    class IshikawaDiagram final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(IshikawaDiagram);
        IshikawaDiagram() = default;

      public:
        /// @brief The style of the fish (its head, tail, eye, and mouth).
        enum class FishStyle
            {
            /// @brief No head, tail, eye, or mouth is drawn.
            None,
            /// @brief A fish with a simple line for the mouth.
            Fish,
            /// @brief A notched mouth with teeth.
            Shark
            };

        /// @brief Sub-causes for a single cause.
        struct SubCauses
            {
            /// @brief The text of the cause that these sub-causes belong to.
            wxString m_cause;
            /// @brief The categorical column containing the sub-causes.
            /// @details If empty, @c m_constants are used instead.
            wxString m_column;
            /// @brief Sub-causes that are entered directly, rather than read from a column.
            std::vector<wxString> m_constants;
            };

        /// @brief A branch of the diagram.
        struct Branch
            {
            /// @brief The branch's label. If empty, a default label is used.
            wxString m_label;
            /// @brief The categorical column containing the branch's causes.
            /// @details If empty, @c m_constants are used instead.
            wxString m_column;
            /// @brief Optional sub-causes for causes on this branch.
            std::vector<SubCauses> m_subCauses;
            /// @brief Causes that are entered directly, rather than read from a column.
            /// @details If this and @c m_column are both empty, the branch is shown
            ///     without any causes.
            std::vector<wxString> m_constants;
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the diagram on.*/
        explicit IshikawaDiagram(Canvas* canvas);

        /** @brief Sets the data.
            @param data The data to use for the diagram.
            @param effect The effect (problem) being analyzed.
            @param branches The branches, each referencing a column of causes.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data, const wxString& effect,
                     const std::vector<Branch>& branches);

        /// @returns The effect (problem) being analyzed.
        [[nodiscard]]
        const wxString& GetEffect() const noexcept
            {
            return m_effect;
            }

        /// @returns The branches that were passed to SetData().
        [[nodiscard]]
        const std::vector<Branch>& GetBranches() const noexcept
            {
            return m_branchSpecs;
            }

        /// @name Display Functions
        /// @brief Functions relating to the display of the diagram.
        /// @{

        /// @brief Sets whether branches that have no causes are shown.
        /// @param show @c true to show empty branches.
        void ShowEmptyBranches(const bool show) noexcept { m_showEmptyBranches = show; }

        /// @returns @c true if branches without causes are shown.
        [[nodiscard]]
        bool IsShowingEmptyBranches() const noexcept
            {
            return m_showEmptyBranches;
            }

        /// @brief Sets whether the branch labels are shown.
        /// @param show @c true to show the branch labels.
        void ShowBranchLabels(const bool show) noexcept { m_showBranchLabels = show; }

        /// @returns @c true if the branch labels are shown.
        [[nodiscard]]
        bool IsShowingBranchLabels() const noexcept
            {
            return m_showBranchLabels;
            }

        /// @brief Sets whether each branch label shows how many causes the branch has.
        /// @param show @c true to show the cause counts (e.g., "People (3)").
        void ShowCauseCounts(const bool show) noexcept { m_showCauseCounts = show; }

        /// @returns @c true if the branch labels show their cause counts.
        [[nodiscard]]
        bool IsShowingCauseCounts() const noexcept
            {
            return m_showCauseCounts;
            }

        /// @brief Sets whether the spine and ribs widen toward the effect, instead of being
        ///     drawn with a constant width.
        /// @param tapered @c true to taper the spine and ribs.
        void SetTaperedLines(const bool tapered) noexcept { m_taperedLines = tapered; }

        /// @returns @c true if the spine and ribs are tapered.
        [[nodiscard]]
        bool IsTaperedLines() const noexcept
            {
            return m_taperedLines;
            }

        /// @brief Sets whether the diagram is drawn in the text color (and shades of it),
        ///     rather than in a color for each branch.
        /// @param monochrome @c true to draw the diagram without branch colors.
        void SetMonochrome(const bool monochrome) noexcept { m_monochrome = monochrome; }

        /// @returns @c true if the diagram is drawn without branch colors.
        [[nodiscard]]
        bool IsMonochrome() const noexcept
            {
            return m_monochrome;
            }

        /// @brief Sets the style of the fish (its head, tail, eye, and mouth).
        /// @param style The fish style.
        void SetFishStyle(const FishStyle style) noexcept { m_fishStyle = style; }

        /// @returns The style of the fish (its head, tail, eye, and mouth).
        [[nodiscard]]
        FishStyle GetFishStyle() const noexcept
            {
            return m_fishStyle;
            }

        /** @brief Splits typed causes into a list.
            @details Causes are separated by spaces, and a cause with several words is enclosed
                in double quotation marks (e.g., `Speed "Operator training"`).
            @param text The typed causes.
            @returns The causes.*/
        [[nodiscard]]
        static std::vector<wxString> ParseConstants(const wxString& text);

        /** @brief Joins causes into the typed format read by ParseConstants().
            @param constants The causes.
            @returns The causes as typed text.*/
        [[nodiscard]]
        static wxString FormatConstants(const std::vector<wxString>& constants);

        /// @param index The index of the branch.
        /// @returns The default label for the branch at @c index, or an empty string
        ///     if the index is beyond the default branches.
        [[nodiscard]]
        static wxString GetDefaultBranchLabel(size_t index);

        /** @brief Determines whether a name (e.g., a column name) matches a default branch label.
            @param index The index of the branch.
            @param name The name to compare, ignoring case. The first branch also matches
                the traditional label "Man".
            @returns @c true if @c name matches the default label for the branch.*/
        [[nodiscard]]
        static bool IsDefaultBranchName(size_t index, const wxString& name);

        /// @brief The number of branches that have a default label.
        constexpr static size_t DEFAULT_BRANCH_COUNT{ 6 };

        /// @}

        /// @private
        [[deprecated("Ishikawa diagram does not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            wxFAIL_MSG(L"Ishikawa diagram does not support legends.");
            return nullptr;
            }

      private:
        struct CauseItem
            {
            wxString m_text;
            std::vector<wxString> m_subCauses;
            };

        struct BranchItem
            {
            wxString m_label;
            std::vector<CauseItem> m_causes;
            };

        // the forked tail fin at the left end of the spine, drawn with quad bezier curves
        class TailFin final : public GraphItems::GraphItemBase
            {
          public:
            /** @brief Constructor.
                @param spinePoint The point where the fin meets the spine.
                @param topControl The control point for the upper lobe's curve.
                @param bottomControl The control point for the lower lobe's curve.
                @param notchPoint The concave notch between the two lobes.
                @param color The fill color of the fin.*/
            TailFin(const wxPoint& spinePoint, const wxPoint& topControl,
                    const wxPoint& bottomControl, const wxPoint& notchPoint, const wxColour& color);

            /// @private
            [[nodiscard]]
            wxRect GetBoundingBox(wxDC& dc) const final;

          private:
            wxRect Draw(wxDC& dc) const final;

            [[nodiscard]]
            bool HitTest(wxPoint pt, wxDC& dc) const final;

            void Offset(int xOffset, int yOffset) final;

            void SetBoundingBox(const wxRect& rect, wxDC& dc, double scaling) final;

            wxPoint m_spinePoint;
            wxPoint m_topControl;
            wxPoint m_bottomControl;
            wxPoint m_notchPoint;
            wxColour m_color;
            };

        // the triangular fish head at the right end of the spine, drawn with rounded corners
        class Head final : public GraphItems::GraphItemBase
            {
          public:
            /** @brief Constructor.
                @param topLeft The top-left vertex of the triangle.
                @param bottomLeft The bottom-left vertex of the triangle.
                @param apex The right-hand apex of the triangle.
                @param color The fill color of the head.
                @param featureColor The color of the eye and mouth.
                @param fishStyle The style of the fish.
                @param scaling The canvas scaling, used to scale the head's corners and pens.*/
            Head(const wxPoint& topLeft, const wxPoint& bottomLeft, const wxPoint& apex,
                 const wxColour& color, const wxColour& featureColor, const FishStyle fishStyle,
                 const double scaling);

            /// @private
            [[nodiscard]]
            wxRect GetBoundingBox(wxDC& dc) const final;

          private:
            wxRect Draw(wxDC& dc) const final;

            [[nodiscard]]
            bool HitTest(wxPoint pt, wxDC& dc) const final;

            void Offset(int xOffset, int yOffset) final;

            void SetBoundingBox(const wxRect& rect, wxDC& dc, double scaling) final;

            wxPoint m_topLeft;
            wxPoint m_bottomLeft;
            wxPoint m_apex;
            wxColour m_color;
            wxColour m_featureColor;
            FishStyle m_fishStyle;
            };

        void SetAutoAccessibilityAttributes() override;
        void RecalcSizes(wxDC& dc) final;

        wxString m_effect;
        std::vector<Branch> m_branchSpecs;
        std::vector<BranchItem> m_branches;
        bool m_showEmptyBranches{ true };
        bool m_showBranchLabels{ true };
        bool m_showCauseCounts{ false };
        bool m_taperedLines{ true };
        bool m_monochrome{ false };
        FishStyle m_fishStyle{ FishStyle::Fish };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // ISHIKAWA_DIAGRAM_H
