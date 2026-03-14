/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_CHERNOFF_PLOT_H
#define WISTERIA_CHERNOFF_PLOT_H

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A Chernoff faces plot, where multivariate data is visualized as
            cartoon faces with facial features mapped to data variables.
        @details Each observation in the dataset is represented as a face,
            where features like face width, eye size, eyebrow angle, nose size,
            mouth width/curvature, etc. are controlled by continuous variables.

            This visualization technique leverages humans' innate ability to recognize
            and differentiate faces, making it easier to spot outliers and patterns
            in high-dimensional data.

        | Parameter | Effect (0 @htmlonly &rarr; @endhtmlonly 1) |
        | :-- | :-- |
        | Face width | Narrow @htmlonly &rarr; @endhtmlonly Wide |
        | Face height | Short @htmlonly &rarr; @endhtmlonly Tall |
        | Eye size | Small @htmlonly &rarr; @endhtmlonly Large |
        | Eye position | Low @htmlonly &rarr; @endhtmlonly High on face |
        | Eyebrow slant | Angled down @htmlonly &rarr; @endhtmlonly Angled up |
        | Pupil position | Looking left @htmlonly &rarr; @endhtmlonly Looking right |
        | Nose size | Small @htmlonly &rarr; @endhtmlonly Large |
        | Mouth width | Narrow @htmlonly &rarr; @endhtmlonly Wide |
        | Mouth curvature | Frown @htmlonly &rarr; @endhtmlonly Smile |
        | Face color | Pale @htmlonly &rarr; @endhtmlonly Saturated |
        | Ear size | Small @htmlonly &rarr; @endhtmlonly Large |

        @par %Data:
            This plot accepts a Data::Dataset where multiple continuous columns
            control the facial features. An optional ID column provides labels
            beneath each face. All continuous values are internally normalized
            to a [0,1] range.

        @par Observation Limits:
            For effective visualization, this chart is limited to 36 observations
            (6x6 grid). Beyond this, faces become too small for meaningful comparison.

        @par Missing Data:
            - Missing data in the ID column results in no label beneath the face.
            - Missing data in feature columns uses a neutral/default value (0.5).
              If all data for an observation is missing, however,
              then the face will not be rendered.

        @par Citation:
            Chernoff, Herman.
            "The Use of Faces to Represent Points in K-Dimensional Space Graphically."
            Journal of the American Statistical Association, vol. 68, no. 342, 1973, pp. 361-368.

        | Chernoff Faces (Female) | Chernoff Faces (Male) |
        | :-------------- | :-------------- |
        | @image html chernoff-female.svg width=90% | @image html chernoff-male.svg width=90% |

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         auto faceData = std::make_shared<Data::Dataset>();
         faceData->ImportCSV(L"multivariate.csv",
            ImportInfo().
            ContinuousColumns({ L"Var1", L"Var2", L"Var3", L"Var4", L"Var5" }).
            IdColumn(L"Name"));

         auto chernoffPlot = std::make_shared<ChernoffFacesPlot>(canvas);
         chernoffPlot->SetData(faceData,
             L"Var1",  // face width
             L"Var2",  // face height
             L"Var3",  // eye size
             L"Var4",  // mouth width
             L"Var5"); // mouth curvature

         canvas->SetFixedObject(0, 0, chernoffPlot);
         canvas->SetFixedObject(0, 1,
            chernoffPlot->CreateLegend(
                LegendOptions().
                    IncludeHeader(true).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        @endcode*/
    class ChernoffFacesPlot final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(ChernoffFacesPlot);
        ChernoffFacesPlot() = default;

      public:
        /// @brief Maximum number of faces that can be displayed effectively.
        constexpr static size_t MAX_FACES = 36;
        /// @brief Default value for missing/unspecified features (normalized).
        constexpr static double DEFAULT_FEATURE_VALUE = math_constants::half;

        /** @brief Constructor.
            @param canvas The canvas that the plot is plotted on.
            @param faceColor The base color for the faces.\n
                Leave as default for a flesh-tone color.*/
        explicit ChernoffFacesPlot(Canvas* canvas,
                                   const wxColour& faceColor = wxColour{ 255, 224, 189 });

        /** @brief Sets the data for the Chernoff faces plot.
            @param data The data to use.
            @param faceWidthColumn Column controlling face width.
            @param faceHeightColumn Column controlling face height (optional).
            @param eyeSizeColumn Column controlling eye size (optional).
            @param eyePositionColumn Column controlling eye vertical position (optional).
            @param eyebrowSlantColumn Column controlling eyebrow angle (optional).
            @param pupilPositionColumn Column controlling pupil horizontal position (optional).
            @param noseSizeColumn Column controlling nose size (optional).
            @param mouthWidthColumn Column controlling mouth width (optional).
            @param mouthCurvatureColumn Column controlling smile/frown (optional).
            @param faceSaturationColumn Column controlling face color saturation (optional).
            @param earSizeColumn Column controlling ear size (optional).
            @warning If the dataset contains more than @c MAX_FACES observations,
                only the first @c MAX_FACES will be displayed and a warning will be logged.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset
                to re-plot the data.
            @throws std::runtime_error If the required column can't be found by name.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& faceWidthColumn,
                     const std::optional<wxString>& faceHeightColumn = std::nullopt,
                     const std::optional<wxString>& eyeSizeColumn = std::nullopt,
                     const std::optional<wxString>& eyePositionColumn = std::nullopt,
                     const std::optional<wxString>& eyebrowSlantColumn = std::nullopt,
                     const std::optional<wxString>& pupilPositionColumn = std::nullopt,
                     const std::optional<wxString>& noseSizeColumn = std::nullopt,
                     const std::optional<wxString>& mouthWidthColumn = std::nullopt,
                     const std::optional<wxString>& mouthCurvatureColumn = std::nullopt,
                     const std::optional<wxString>& faceSaturationColumn = std::nullopt,
                     const std::optional<wxString>& earSizeColumn = std::nullopt);

        /// @name Appearance Functions
        /// @brief Functions relating to the visual appearance of the faces.
        /// @{

        /// @returns The base face color.
        [[nodiscard]]
        wxColour GetFaceColor() const noexcept
            {
            return m_faceColor;
            }

        /// @brief Sets the base face color.
        /// @param color The color for the faces.
        void SetFaceColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_faceColor = color;
                }
            }

        /// @brief Sets the skin color range for face saturation.
        /// @details When face saturation data is mapped, values interpolate between
        ///     these two colors (0 = lighter, 1 = darker).
        /// @param lighter The lighter skin color (low saturation).
        /// @param darker The darker skin color (high saturation).
        void SetSkinColorRange(const wxColour& lighter, const wxColour& darker) noexcept
            {
            if (lighter.IsOk())
                {
                m_faceColorLighter = lighter;
                }
            if (darker.IsOk())
                {
                m_faceColor = darker;
                }
            }

        /// @returns Whether labels are shown beneath each face.
        /// @note The dataset must have an ID column to pull the labels from;
        ///     otherwise, this will have no effect.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @brief Sets whether to show ID labels beneath each face.
        /// @param show @c true to show labels.
        /// @note The dataset must have an ID column to pull the labels from;
        ///     otherwise, this will have no effect.
        void ShowLabels(const bool show) noexcept { m_showLabels = show; }

        /// @returns The outline color for facial features.
        [[nodiscard]]
        wxColour GetOutlineColor() const noexcept
            {
            return m_outlineColor;
            }

        /// @brief Sets the outline color for facial features.
        /// @param color The outline color.
        void SetOutlineColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_outlineColor = color;
                }
            }

        /// @returns The gender used for face styling.
        [[nodiscard]]
        Gender GetGender() const noexcept
            {
            return m_gender;
            }

        /// @brief Sets the gender used for face styling.
        /// @param gender The gender to use.
        void SetGender(const Gender gender) noexcept { m_gender = gender; }

        /// @returns The lipstick color (only used for female faces).
        [[nodiscard]]
        wxColour GetLipstickColor() const noexcept
            {
            return m_lipstickColor;
            }

        /// @brief Sets the lipstick color (only used for female faces).
        /// @param color The lipstick color.
        /// @note This is not connected to any variable and is only used for aesthetic purposes.
        void SetLipstickColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_lipstickColor = color;
                }
            }

        /// @returns The eye color.
        [[nodiscard]]
        wxColour GetEyeColor() const noexcept
            {
            return m_eyeColor;
            }

        /// @brief Sets the eye color.
        /// @param color The eye color.
        /// @note This is not connected to any variable and is only used for aesthetic purposes.
        void SetEyeColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_eyeColor = color;
                }
            }

        /// @returns The hair style (female faces only).
        [[nodiscard]]
        HairStyle GetHairStyle() const noexcept
            {
            return m_hairStyle;
            }

        /// @brief Sets the hair style (female faces only).
        /// @param style The hair style to use.
        void SetHairStyle(const HairStyle style) noexcept { m_hairStyle = style; }

        /// @returns The hair color.
        [[nodiscard]]
        wxColour GetHairColor() const noexcept
            {
            return m_hairColor;
            }

        /// @brief Sets the hair color.
        /// @param color The hair color.
        void SetHairColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_hairColor = color;
                }
            }

        /// @returns The facial hair style (male faces only).
        [[nodiscard]]
        FacialHair GetFacialHair() const noexcept
            {
            return m_facialHair;
            }

        /// @brief Sets the facial hair style (male faces only).
        /// @param style The facial hair style to use.
        void SetFacialHair(const FacialHair style) noexcept { m_facialHair = style; }

        /// @}

        /// @brief Identifiers for facial features.
        /// @private
        enum class FeatureId
            {
            FaceWidth,
            FaceHeight,
            EyeSize,
            EyePosition,
            EyebrowSlant,
            PupilDirection,
            NoseSize,
            MouthWidth,
            SmileFrown,
            FaceColor,
            EarSize
            };

        /// @brief Returns the localized display name for a feature.
        /// @param id The feature identifier.
        /// @returns The localized string for the feature.
        [[nodiscard]]
        static wxString GetFeatureDisplayName(FeatureId id);

        /// @name Legend
        /// @brief Legend-related classes and functions.
        /// @{

        /// @brief A legend object showing a face with labeled connection lines to features.
        class ChernoffLegend final : public GraphItems::GraphItemBase
            {
            wxDECLARE_DYNAMIC_CLASS(ChernoffLegend);

          public:
            /** @brief Constructor.
                @param itemInfo The base item information.*/
            explicit ChernoffLegend(const GraphItems::GraphItemInfo& itemInfo)
                : GraphItemBase(itemInfo)
                {
                }

            /// @private
            ChernoffLegend() = default;
            /// @private
            ChernoffLegend(const ChernoffLegend&) = delete;
            /// @private
            ChernoffLegend& operator=(const ChernoffLegend&) = delete;

            /// @brief Structure describing a feature label and its position.
            struct FeatureLabel
                {
                wxString columnName; ///< data column name (empty = not in use)
                FeatureId featureId{ FeatureId::FaceWidth }; ///< feature identifier
                bool leftSide{ true }; ///< whether label is on left or right side
                };

            [[nodiscard]]
            wxRect Draw(wxDC& dc) const final;
            [[nodiscard]]
            wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
            void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                                [[maybe_unused]] double parentScaling) final;
            void RecalcSizes(wxDC& dc) final;

            /// @brief Sets the features to display in the legend.
            /// @param features The feature labels to show.
            void SetFeatures(std::vector<FeatureLabel> features) noexcept
                {
                m_features = std::move(features);
                }

            /// @brief Sets the face skin colors.
            /// @param lighter The lighter skin color.
            /// @param darker The darker skin color.
            void SetFaceColors(const wxColour& lighter, const wxColour& darker) noexcept
                {
                m_faceColorLighter = lighter;
                m_faceColorDarker = darker;
                }

            /// @brief Sets the outline color.
            /// @param color The outline color.
            void SetOutlineColor(const wxColour& color) noexcept { m_outlineColor = color; }

            /// @brief Sets the gender for face styling.
            /// @param gender The gender.
            void SetGender(Gender gender) noexcept { m_gender = gender; }

            /// @brief Sets the hair style.
            /// @param style The hair style.
            void SetHairStyle(HairStyle style) noexcept { m_hairStyle = style; }

            /// @brief Sets the hair color.
            /// @param color The hair color.
            void SetHairColor(const wxColour& color) noexcept { m_hairColor = color; }

            /// @brief Sets the eye color.
            /// @param color The eye color.
            void SetEyeColor(const wxColour& color) noexcept { m_eyeColor = color; }

            /// @brief Sets the lipstick color (female faces only).
            /// @param color The lipstick color.
            void SetLipstickColor(const wxColour& color) noexcept { m_lipstickColor = color; }

            /// @brief Sets the facial hair style (male faces only).
            /// @param style The facial hair style.
            void SetFacialHair(FacialHair style) noexcept { m_facialHair = style; }

            /// @brief Sets the pen for connection lines.
            /// @param pen The pen.
            void SetConnectionLinePen(const wxPen& pen) noexcept { m_connectionLinePen = pen; }

            /// @brief Sets the canvas background color used for calculating contrast.
            /// @details This color is not drawn; it is only used to determine appropriate
            ///     tinting for connection line arrows and label font colors.
            /// @param color The canvas background color.
            void SetCanvasBackgroundColor(const wxColour& color) noexcept
                {
                m_canvasBackgroundColor = color;
                }

          private:
            void Offset(const int xToMove, const int yToMove) final
                {
                m_rect.Offset(xToMove, yToMove);
                }

            [[nodiscard]]
            bool HitTest([[maybe_unused]] const wxPoint pt, [[maybe_unused]] wxDC& dc) const final
                {
                return false;
                }

            std::vector<FeatureLabel> m_features;
            wxColour m_faceColorLighter{ 255, 239, 219 };
            wxColour m_faceColorDarker{ 255, 224, 189 };
            wxColour m_outlineColor{ *wxBLACK };
            wxColour m_lipstickColor{ 178, 34, 34 };
            wxColour m_eyeColor{ 143, 188, 143 };
            wxColour m_hairColor{ 183, 82, 46 };
            Gender m_gender{ Gender::Female };
            HairStyle m_hairStyle{ HairStyle::Bob };
            FacialHair m_facialHair{ FacialHair::CleanShaven };
            wxPen m_connectionLinePen{ *wxBLACK, 1 };
            wxColour m_canvasBackgroundColor{ *wxWHITE };
            wxRect m_rect;
            };

        /** @brief Builds and returns a legend explaining the feature mappings.
            @details The legend lists each facial feature and the column it maps to.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

        /** @brief Builds and returns an extended legend with a face graphic and labeled features.
            @details The legend shows a face with connection lines pointing to each feature.
            @param options The options for how to build the legend.
            @returns The graphical legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<ChernoffLegend> CreateExtendedLegend(const LegendOptions& options);

        /// @}

      private:
        /// @brief Internal structure holding normalized feature values for one face.
        struct FaceFeatures
            {
            double faceWidth{ DEFAULT_FEATURE_VALUE };
            double faceHeight{ DEFAULT_FEATURE_VALUE };
            double eyeSize{ DEFAULT_FEATURE_VALUE };
            double eyePosition{ DEFAULT_FEATURE_VALUE };
            double eyebrowSlant{ DEFAULT_FEATURE_VALUE };
            double pupilPosition{ DEFAULT_FEATURE_VALUE };
            double noseSize{ DEFAULT_FEATURE_VALUE };
            double mouthWidth{ DEFAULT_FEATURE_VALUE };
            double mouthCurvature{ DEFAULT_FEATURE_VALUE };
            double faceSaturation{ DEFAULT_FEATURE_VALUE };
            double earSize{ DEFAULT_FEATURE_VALUE };
            wxString label;
            bool allDataMissing{ false };
            };

        /// @brief A drawable face object.
        class FaceObject final : public GraphItems::GraphItemBase
            {
          public:
            FaceObject(const GraphItems::GraphItemInfo& itemInfo, FaceFeatures features,
                       const wxSize& sz, const wxColour& faceColorLighter,
                       const wxColour& faceColorDarker, const wxColour& outlineColor,
                       const wxColour& lipstickColor, const wxColour& eyeColor,
                       const wxColour& hairColor, const HairStyle hairStyle, const Gender gender,
                       const FacialHair facialHair)
                : GraphItemBase(itemInfo), m_features(std::move(features)), m_size(sz),
                  m_faceColorLighter(faceColorLighter), m_faceColorDarker(faceColorDarker),
                  m_outlineColor(outlineColor), m_lipstickColor(lipstickColor),
                  m_eyeColor(eyeColor), m_hairColor(hairColor), m_hairStyle(hairStyle),
                  m_gender(gender), m_facialHair(facialHair)
                {
                }

            /// @private
            FaceObject(const FaceObject&) = delete;
            /// @private
            FaceObject& operator=(const FaceObject&) = delete;

            [[nodiscard]]
            wxRect Draw(wxDC& dc) const final;
            [[nodiscard]]
            wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final;
            void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                                [[maybe_unused]] double parentScaling) final;

          private:
            void Offset(const int xToMove, const int yToMove) final
                {
                SetAnchorPoint(GetAnchorPoint() + wxPoint(xToMove, yToMove));
                }

            [[nodiscard]]
            bool HitTest([[maybe_unused]] const wxPoint pt, [[maybe_unused]] wxDC& dc) const final
                {
                return false;
                }

            FaceFeatures m_features;
            wxSize m_size;
            wxColour m_faceColorLighter;
            wxColour m_faceColorDarker;
            wxColour m_outlineColor;
            wxColour m_lipstickColor;
            wxColour m_eyeColor;
            wxColour m_hairColor;
            HairStyle m_hairStyle{ HairStyle::Bob };
            Gender m_gender{ Gender::Female };
            FacialHair m_facialHair{ FacialHair::CleanShaven };
            };

        void RecalcSizes(wxDC& dc) final;

        /// @brief Draws a single face within the specified rectangle.
        /// @param gc The graphics context to draw to.
        /// @param rect The bounding rectangle for the face.
        /// @param features The normalized feature values.
        /// @param faceColorLighter The lighter skin color (low saturation).
        /// @param faceColorDarker The darker skin color (high saturation).
        /// @param outlineColor The outline color.
        /// @param lipstickColor The lipstick color (female only).
        /// @param eyeColor The eye/iris color.
        /// @param hairColor The hair color.
        /// @param hairStyle The hair style.
        /// @param gender The gender for face styling.
        /// @param facialHair The facial hair style (male only).
        static void DrawFace(wxGraphicsContext* gc, const wxRect& rect,
                             const FaceFeatures& features, const wxColour& faceColorLighter,
                             const wxColour& faceColorDarker, const wxColour& outlineColor,
                             const wxColour& lipstickColor, const wxColour& eyeColor,
                             const wxColour& hairColor, HairStyle hairStyle, Gender gender,
                             FacialHair facialHair);

        /// @brief Normalizes a value to the [0,1] range.
        /// @param value The raw value.
        /// @param minVal The minimum value in the column.
        /// @param maxVal The maximum value in the column.
        /// @returns The normalized value, or @c DEFAULT_FEATURE_VALUE if invalid.
        [[nodiscard]]
        static double NormalizeValue(double value, double minVal, double maxVal) noexcept;

        /// @brief Calculates min/max for a column.
        /// @param columnName The column name.
        /// @param outMin Output minimum value.
        /// @param outMax Output maximum value.
        void CalculateColumnRange(const wxString& columnName, double& outMin, double& outMax) const;

        std::vector<FaceFeatures> m_faces;
        wxColour m_faceColor{ 255, 224, 189 };
        wxColour m_faceColorLighter{ 255, 239, 219 };
        wxColour m_outlineColor{ *wxBLACK };
        wxColour m_lipstickColor{ 178, 34, 34 };
        wxColour m_eyeColor{ 143, 188, 143 }; // light green
        wxColour m_hairColor{ 183, 82, 46 };  // natural auburn/ginger
        bool m_showLabels{ true };
        Gender m_gender{ Gender::Female };
        HairStyle m_hairStyle{ HairStyle::Bob };
        FacialHair m_facialHair{ FacialHair::CleanShaven };

        // column names for legend
        wxString m_faceWidthColumnName;
        wxString m_faceHeightColumnName;
        wxString m_eyeSizeColumnName;
        wxString m_eyePositionColumnName;
        wxString m_eyebrowSlantColumnName;
        wxString m_pupilPositionColumnName;
        wxString m_noseSizeColumnName;
        wxString m_mouthWidthColumnName;
        wxString m_mouthCurvatureColumnName;
        wxString m_faceSaturationColumnName;
        wxString m_earSizeColumnName;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_CHERNOFF_PLOT_H
