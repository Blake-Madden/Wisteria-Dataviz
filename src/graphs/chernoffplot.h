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
        | Hair style | Category string mapped to HairStyleFemale or HairStyleMale |
        | Hair addition | Category string mapped to FacialHair (male) or HairAccessory (female) |

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
            @param hairStyleColumn Optional categorical column controlling per-face hair style.\n
           Category strings are mapped to HairStyleFemale or HairStyleMale values (depending on
           the plot's gender) in the order they appear in the data.\n When this column is not
           provided, every face uses the plot-wide hair style set via SetHairStyle().
            @param hairAdditionColumn Optional categorical column controlling per-face hair
           addition.\n For male faces, category strings are mapped to FacialHair values (e.g., @c
           "beard", @c "mustache").\n For female faces, category strings are mapped to HairAccessory
           values (e.g., @c "flower", @c "star").\n Unrecognized strings default to
           FacialHair::CleanShaven or HairAccessory::None.
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
                     const std::optional<wxString>& earSizeColumn = std::nullopt,
                     const std::optional<wxString>& hairStyleColumn = std::nullopt,
                     const std::optional<wxString>& hairAdditionColumn = std::nullopt);

        /// @name Appearance Functions
        /// @brief Functions relating to the visual appearance of the faces.
        /// @{

        /// @returns The base (darker) face color.
        [[nodiscard]]
        wxColour GetFaceColor() const noexcept
            {
            return m_faceColor;
            }

        /// @returns The lighter face color used for low saturation values.
        [[nodiscard]]
        wxColour GetFaceColorLighter() const noexcept
            {
            return m_faceColorLighter;
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

        /// @returns The hair style used for female faces.
        [[nodiscard]]
        HairStyleFemale GetHairStyleFemale() const noexcept
            {
            return m_hairStyleFemale;
            }

        /// @returns The hair style used for male faces.
        [[nodiscard]]
        HairStyleMale GetHairStyleMale() const noexcept
            {
            return m_hairStyleMale;
            }

        /// @brief Sets the hair style used for female faces.
        /// @param style The hair style to use.
        void SetHairStyle(const HairStyleFemale style) noexcept { m_hairStyleFemale = style; }

        /// @brief Sets the hair style used for male faces.
        /// @param style The hair style to use.
        void SetHairStyle(const HairStyleMale style) noexcept { m_hairStyleMale = style; }

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
            EarSize,
            HairStyle,
            HairAddition
            };

        /// @brief Returns the localized display name for a feature.
        /// @param id The feature identifier.
        /// @returns The localized string for the feature.
        [[nodiscard]]
        static wxString GetFeatureDisplayName(FeatureId id);

        /// @brief Returns the column name assigned to a feature.
        /// @param id The feature identifier.
        /// @returns The column name, or an empty string if not assigned.
        [[nodiscard]]
        wxString GetFeatureColumnName(FeatureId id) const;

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
                wxString m_columnName; ///< data column name (empty = not in use)
                FeatureId m_featureId{ FeatureId::FaceWidth }; ///< feature identifier
                bool m_leftSide{ true }; ///< whether label is on left or right side
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

            /// @brief Sets the hair style used for female faces.
            /// @param style The hair style.
            void SetHairStyle(HairStyleFemale style) noexcept { m_hairStyleFemale = style; }

            /// @brief Sets the hair style used for male faces.
            /// @param style The hair style.
            void SetHairStyle(HairStyleMale style) noexcept { m_hairStyleMale = style; }

            /// @brief Sets the hair color.
            /// @param color The hair color.
            void SetHairColor(const wxColour& color) noexcept { m_hairColor = color; }

            /// @brief Sets the eye color.
            /// @param color The eye color.
            void SetEyeColor(const wxColour& color) noexcept { m_eyeColor = color; }

            /// @brief Sets the lipstick color.
            /// @param color The lipstick color.
            void SetLipstickColor(const wxColour& color) noexcept { m_lipstickColor = color; }

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

            /// @brief Sets the hair-style factor labels (ordered by enum index).
            /// @details When non-empty, the legend renders an additional "key" section
            ///     below the face listing each label with a small face icon showing
            ///     that hair style.
            /// @param labels The label list, where index @c N corresponds to
            ///     @c HairStyleFemale(N) (for female faces) or @c HairStyleMale(N) (for male).
            void SetHairStyleLabels(std::vector<wxString> labels) noexcept
                {
                m_hairStyleLabels = std::move(labels);
                }

            /// @brief Sets the hair-style column name (used as the key section header).
            /// @param name The column name.
            void SetHairStyleColumnName(wxString name) noexcept
                {
                m_hairStyleColumnName = std::move(name);
                }

            /// @brief Sets the hair-addition factor labels (ordered by enum index).
            /// @details When non-empty, the legend renders an additional "key" section
            ///     below the face listing each label with its corresponding hair icon
            ///     (facial hair for male, hair accessory for female).
            /// @param labels The label list, where index @c N corresponds to
            ///     @c FacialHair(N) / @c HairAccessory(N).
            void SetHairAdditionLabels(std::vector<wxString> labels) noexcept
                {
                m_hairAdditionLabels = std::move(labels);
                }

            /// @brief Sets the hair-addition column name (used as the key section header).
            /// @param name The column name.
            void SetHairAdditionColumnName(wxString name) noexcept
                {
                m_hairAdditionColumnName = std::move(name);
                }

          private:
            void Offset(const int xToMove, const int yToMove) final
                {
                m_rect.Offset(xToMove, yToMove);
                }

            [[nodiscard]]
            bool HitTest(const wxPoint pt, wxDC& dc) const final
                {
                return GetBoundingBox(dc).Contains(pt);
                }

            std::vector<FeatureLabel> m_features;
            std::vector<wxString> m_hairStyleLabels;
            wxString m_hairStyleColumnName;
            std::vector<wxString> m_hairAdditionLabels;
            wxString m_hairAdditionColumnName;
            wxColour m_faceColorLighter{ 255, 239, 219 };
            wxColour m_faceColorDarker{ 255, 224, 189 };
            wxColour m_outlineColor{ *wxBLACK };
            wxColour m_lipstickColor{ 178, 34, 34 };
            wxColour m_eyeColor{ 143, 188, 143 };
            wxColour m_hairColor{ 183, 82, 46 };
            Gender m_gender{ Gender::Female };
            HairStyleFemale m_hairStyleFemale{ HairStyleFemale::Bob };
            HairStyleMale m_hairStyleMale{ HairStyleMale::CombOver };
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

        /** @brief Builds and returns an enhanced legend with a face graphic and labeled features.
            @details The legend shows a face with connection lines pointing to each feature.
            @param options The options for how to build the legend.
            @returns The graphical legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<ChernoffLegend> CreateEnhancedLegend(const LegendOptions& options);

        /// @brief The type of legend most recently created for this plot.
        enum class LegendType
            {
            Regular, /*!< @brief A standard Label-based legend.*/
            Enhanced /*!< @brief An extended ChernoffLegend with face graphic.*/
            };

        /// @returns The legend type that was last created.
        [[nodiscard]]
        LegendType GetLastLegendType() const noexcept
            {
            return m_lastLegendType;
            }

        /// @}

      private:
        /// @brief Internal structure holding normalized feature values for one face.
        struct FaceFeatures
            {
            double m_faceWidth{ DEFAULT_FEATURE_VALUE };
            double m_faceHeight{ DEFAULT_FEATURE_VALUE };
            double m_eyeSize{ DEFAULT_FEATURE_VALUE };
            double m_eyePosition{ DEFAULT_FEATURE_VALUE };
            double m_eyebrowSlant{ DEFAULT_FEATURE_VALUE };
            double m_pupilPosition{ DEFAULT_FEATURE_VALUE };
            double m_noseSize{ DEFAULT_FEATURE_VALUE };
            double m_mouthWidth{ DEFAULT_FEATURE_VALUE };
            double m_mouthCurvature{ DEFAULT_FEATURE_VALUE };
            double m_faceSaturation{ DEFAULT_FEATURE_VALUE };
            double m_earSize{ DEFAULT_FEATURE_VALUE };
            HairStyleFemale m_hairStyleFemale{ HairStyleFemale::Bob };
            HairStyleMale m_hairStyleMale{ HairStyleMale::CombOver };
            FacialHair m_facialHair{ FacialHair::CleanShaven };
            HairAccessory m_hairAccessory{ HairAccessory::Butterfly };
            /// @brief Whether @c m_hairAccessory should be rendered. Set to @c true
            ///     only when a hair-addition column is mapped for this face.
            bool m_hasHairAccessory{ false };
            wxString m_label;
            bool m_allDataMissing{ false };
            };

        /// @brief A drawable face object.
        class FaceObject final : public GraphItems::GraphItemBase
            {
          public:
            FaceObject(const GraphItems::GraphItemInfo& itemInfo, FaceFeatures features,
                       const wxSize& sz, const wxColour& faceColorLighter,
                       const wxColour& faceColorDarker, const wxColour& outlineColor,
                       const wxColour& lipstickColor, const wxColour& eyeColor,
                       const wxColour& hairColor, const HairStyleFemale hairStyleFemale,
                       const HairStyleMale hairStyleMale, const Gender gender)
                : GraphItemBase(itemInfo), m_features(std::move(features)), m_size(sz),
                  m_faceColorLighter(faceColorLighter), m_faceColorDarker(faceColorDarker),
                  m_outlineColor(outlineColor), m_lipstickColor(lipstickColor),
                  m_eyeColor(eyeColor), m_hairColor(hairColor), m_hairStyleFemale(hairStyleFemale),
                  m_hairStyleMale(hairStyleMale), m_gender(gender)
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
            HairStyleFemale m_hairStyleFemale{ HairStyleFemale::Bob };
            HairStyleMale m_hairStyleMale{ HairStyleMale::CombOver };
            Gender m_gender{ Gender::Female };
            };

        void RecalcSizes(wxDC& dc) final;

        void SetAutoAccessibilityAttributes() final;

        /// @brief Which parts of the face to include when calling DrawFace().
        /// @details Each flag defaults to @c true; set fields to @c false to skip
        ///     drawing that part (useful for minimal/legend-icon renderings).
        struct FaceParts
            {
            bool m_face{ true };       ///< face oval + ears + skin color
            bool m_cheeks{ true };     ///< rosy cheeks (female only)
            bool m_eyes{ true };       ///< eyes, pupils, eyelashes, eyebrows
            bool m_nose{ true };       ///< nose
            bool m_mouth{ true };      ///< mouth and lips
            bool m_facialHair{ true }; ///< facial hair (male only)
            bool m_hair{ true };       ///< head hair
            };

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
        /// @param hairStyleFemale The hair style to use when @p gender is @c Female.
        /// @param hairStyleMale The hair style to use when @p gender is @c Male.
        /// @param gender The gender for face styling.
        /// @param parts Which parts of the face to draw.
        static void DrawFace(wxGraphicsContext* gc, const wxRect& rect,
                             const FaceFeatures& features, const wxColour& faceColorLighter,
                             const wxColour& faceColorDarker, const wxColour& outlineColor,
                             const wxColour& lipstickColor, const wxColour& eyeColor,
                             const wxColour& hairColor, HairStyleFemale hairStyleFemale,
                             HairStyleMale hairStyleMale, Gender gender, const FaceParts& parts);

        /// @brief Draws a single face within the specified rectangle, including all parts.
        /// @details Overload that draws every face part; see the other @c DrawFace() for
        ///     parameter details.
        static void DrawFace(wxGraphicsContext* gc, const wxRect& rect,
                             const FaceFeatures& features, const wxColour& faceColorLighter,
                             const wxColour& faceColorDarker, const wxColour& outlineColor,
                             const wxColour& lipstickColor, const wxColour& eyeColor,
                             const wxColour& hairColor, HairStyleFemale hairStyleFemale,
                             HairStyleMale hairStyleMale, Gender gender);

        /// @brief Internal hair-style kind used by the drawing helpers.
        /// @details Collapses the gender-specific @c HairStyleFemale and @c HairStyleMale
        ///     enums to a single kind so the drawing branches don't need to be duplicated
        ///     per gender.
        /// @internal
        enum class HairStyleKind
            {
            Bob,           // female only
            Pixie,         // female only
            Bun,           // female only
            PartiallyBald, // male only
            BaldCombOver,  // male only
            CombOver,      // male only
            LongStraight,  // shared
            HighTopFade,   // shared
            FlatTop,       // shared
            Curly,         // shared
            LongCurly,     // shared
            Bald           // shared
            };

        /// @brief Shared geometry computed once at the top of @c DrawFace and passed to
        ///     each drawing helper.
        struct FaceGeometry
            {
            double m_cx{ 0.0 };
            double m_cy{ 0.0 };
            double m_baseRadius{ 0.0 };
            double m_faceWidth{ 0.0 };
            double m_faceHeight{ 0.0 };
            double m_mouthY{ 0.0 };
            double m_mouthWidthVal{ 0.0 };
            wxColour m_faceCol;
            wxPen m_outlinePen;
            };

        [[nodiscard]]
        static HairStyleKind ToHairStyleKind(HairStyleFemale hsFemale, HairStyleMale hsMale,
                                             Gender gender) noexcept;

        [[nodiscard]]
        static FaceGeometry ComputeFaceGeometry(const wxRect& rect, const FaceFeatures& features,
                                                const wxColour& faceColorLighter,
                                                const wxColour& faceColorDarker,
                                                const wxColour& outlineColor);

        static void DrawFaceShape(wxGraphicsContext* gc, const FaceGeometry& geom,
                                  const FaceFeatures& features);

        static void DrawCheeks(wxGraphicsContext* gc, const FaceGeometry& geom);

        static void DrawFacialHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                   const FaceFeatures& features, const wxColour& outlineColor,
                                   const wxColour& hairColor);

        static void DrawHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                             const FaceFeatures& features, const wxColour& hairColor,
                             HairStyleKind hairStyle);

        /// @brief Draws the @c Bob hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        /// @param browYLimit The y-coordinate that bangs must stay above (the eyebrow line).
        static void DrawBobHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                const wxColour& hairColor, double browYLimit);

        /// @brief Draws the @c Pixie hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        /// @param browYLimit The y-coordinate that bangs must stay above (the eyebrow line).
        static void DrawPixieHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                  const wxColour& hairColor, double browYLimit);

        /// @brief Draws the @c LongStraight hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        /// @param browYLimit The y-coordinate that bangs must stay above (the eyebrow line).
        static void DrawLongStraightHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                         const wxColour& hairColor, double browYLimit);

        /// @brief Draws the @c Bun hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        static void DrawBunHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                const wxColour& hairColor);

        /// @brief Draws the @c Curly and @c LongCurly hair styles.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        /// @param browYLimit The y-coordinate that bangs must stay above (the eyebrow line).
        /// @param hairStyle Either @c Curly or @c LongCurly (controls volume and length).
        static void DrawCurlyHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                  const wxColour& hairColor, double browYLimit,
                                  HairStyleKind hairStyle);

        /// @brief Draws the @c HighTopFade hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        static void DrawHighTopFadeHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                        const wxColour& hairColor);

        /// @brief Draws the @c FlatTop hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        static void DrawFlatTopHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                    const wxColour& hairColor);

        /// @brief Draws the @c PartiallyBald and @c BaldCombOver hair styles.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        /// @param hairStyle Either @c PartiallyBald or @c BaldCombOver (the latter adds
        ///     comb-over strands across the bald crown).
        static void DrawPartiallyBaldHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                          const wxColour& hairColor, HairStyleKind hairStyle);

        /// @brief Draws the @c CombOver hair style.
        /// @param gc The graphics context to draw to.
        /// @param geom The shared face geometry.
        /// @param hairColor The hair color.
        static void DrawCombOverHair(wxGraphicsContext* gc, const FaceGeometry& geom,
                                     const wxColour& hairColor);

        static void DrawEyes(wxGraphicsContext* gc, const FaceGeometry& geom,
                             const FaceFeatures& features, const wxColour& outlineColor,
                             const wxColour& eyeColor, Gender gender);

        static void DrawNose(wxGraphicsContext* gc, const FaceGeometry& geom,
                             const FaceFeatures& features);

        static void DrawMouth(wxGraphicsContext* gc, const FaceGeometry& geom,
                              const FaceFeatures& features, const wxColour& outlineColor,
                              const wxColour& lipstickColor, Gender gender);

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
        HairStyleFemale m_hairStyleFemale{ HairStyleFemale::Bob };
        HairStyleMale m_hairStyleMale{ HairStyleMale::CombOver };

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
        wxString m_hairStyleColumnName;
        wxString m_hairAdditionColumnName;

        // hair-style factor labels, ordered by enum index
        // (index N corresponds to HairStyleFemale(N) or HairStyleMale(N), depending on gender).
        std::vector<wxString> m_hairStyleLabels;

        // hair-addition factor labels, ordered by enum index
        // (index N corresponds to FacialHair(N) and HairAccessory(N)).
        std::vector<wxString> m_hairAdditionLabels;

        LegendType m_lastLegendType{ LegendType::Enhanced };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_CHERNOFF_PLOT_H
