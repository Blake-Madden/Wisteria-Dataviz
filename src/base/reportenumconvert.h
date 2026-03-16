/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_ENUM_CONVERT_H
#define WISTERIA_ENUM_CONVERT_H

#include "../base/icons.h"
#include "../base/shapes.h"
#include "../graphs/boxplot.h"
#include "../graphs/candlestickplot.h"
#include "../graphs/ganttchart.h"
#include "../graphs/histogram.h"
#include "../graphs/likertchart.h"
#include "../graphs/piechart.h"
#include "../graphs/proconroadmap.h"
#include "enums.h"
#include <wx/wx.h>

namespace Wisteria
    {
    /// @private
    class ReportEnumConvert
        {
        // NOLINTBEGIN
        inline static const std::map<std::wstring, wxPaperSize> m_paperSizeValues = {
            { L"paper-letter", wxPaperSize::wxPAPER_LETTER },
            { L"paper-legal", wxPaperSize::wxPAPER_LEGAL },
            { L"paper-a4", wxPaperSize::wxPAPER_A4 },
            { L"paper-csheet", wxPaperSize::wxPAPER_CSHEET },
            { L"paper-dsheet", wxPaperSize::wxPAPER_DSHEET },
            { L"paper-esheet", wxPaperSize::wxPAPER_ESHEET },
            { L"paper-lettersmall", wxPaperSize::wxPAPER_LETTERSMALL },
            { L"paper-tabloid", wxPaperSize::wxPAPER_TABLOID },
            { L"paper-ledger", wxPaperSize::wxPAPER_LEDGER },
            { L"paper-statement", wxPaperSize::wxPAPER_STATEMENT },
            { L"paper-executive", wxPaperSize::wxPAPER_EXECUTIVE },
            { L"paper-a3", wxPaperSize::wxPAPER_A3 },
            { L"paper-a4small", wxPaperSize::wxPAPER_A4SMALL },
            { L"paper-a5", wxPaperSize::wxPAPER_A5 },
            { L"paper-b4", wxPaperSize::wxPAPER_B4 },
            { L"paper-b5", wxPaperSize::wxPAPER_B5 },
            { L"paper-folio", wxPaperSize::wxPAPER_FOLIO },
            { L"paper-quarto", wxPaperSize::wxPAPER_QUARTO },
            { L"paper-10x14", wxPaperSize::wxPAPER_10X14 },
            { L"paper-11x17", wxPaperSize::wxPAPER_11X17 },
            { L"paper-note", wxPaperSize::wxPAPER_NOTE },
            { L"paper-env-9", wxPaperSize::wxPAPER_ENV_9 },
            { L"paper-env-10", wxPaperSize::wxPAPER_ENV_10 },
            { L"paper-env-11", wxPaperSize::wxPAPER_ENV_11 },
            { L"paper-env-12", wxPaperSize::wxPAPER_ENV_12 },
            { L"paper-env-14", wxPaperSize::wxPAPER_ENV_14 },
            { L"paper-env-dl", wxPaperSize::wxPAPER_ENV_DL },
            { L"paper-env-c5", wxPaperSize::wxPAPER_ENV_C5 },
            { L"paper-env-c3", wxPaperSize::wxPAPER_ENV_C3 },
            { L"paper-env-c4", wxPaperSize::wxPAPER_ENV_C4 },
            { L"paper-env-c6", wxPaperSize::wxPAPER_ENV_C6 },
            { L"paper-env-c65", wxPaperSize::wxPAPER_ENV_C65 },
            { L"paper-env-b4", wxPaperSize::wxPAPER_ENV_B4 },
            { L"paper-env-b5", wxPaperSize::wxPAPER_ENV_B5 },
            { L"paper-env-b6", wxPaperSize::wxPAPER_ENV_B6 },
            { L"paper-env-italy", wxPaperSize::wxPAPER_ENV_ITALY },
            { L"paper-env-monarch", wxPaperSize::wxPAPER_ENV_MONARCH },
            { L"paper-env-personal", wxPaperSize::wxPAPER_ENV_PERSONAL },
            { L"paper-fanfold-us", wxPaperSize::wxPAPER_FANFOLD_US },
            { L"paper-fanfold-std-german", wxPaperSize::wxPAPER_FANFOLD_STD_GERMAN },
            { L"paper-fanfold-lgl-german", wxPaperSize::wxPAPER_FANFOLD_LGL_GERMAN },
            { L"paper-iso-b4", wxPaperSize::wxPAPER_ISO_B4 },
            { L"paper-japanese-postcard", wxPaperSize::wxPAPER_JAPANESE_POSTCARD },
            { L"paper-9x11", wxPaperSize::wxPAPER_9X11 },
            { L"paper-10x11", wxPaperSize::wxPAPER_10X11 },
            { L"paper-15x11", wxPaperSize::wxPAPER_15X11 },
            { L"paper-env-invite", wxPaperSize::wxPAPER_ENV_INVITE },
            { L"paper-letter-extra", wxPaperSize::wxPAPER_LETTER_EXTRA },
            { L"paper-legal-extra", wxPaperSize::wxPAPER_LEGAL_EXTRA },
            { L"paper-tabloid-extra", wxPaperSize::wxPAPER_TABLOID_EXTRA },
            { L"paper-a4-extra", wxPaperSize::wxPAPER_A4_EXTRA },
            { L"paper-letter-transverse", wxPaperSize::wxPAPER_LETTER_TRANSVERSE },
            { L"paper-a4-transverse", wxPaperSize::wxPAPER_A4_TRANSVERSE },
            { L"paper-letter-extra-transverse", wxPaperSize::wxPAPER_LETTER_EXTRA_TRANSVERSE },
            { L"paper-a-plus", wxPaperSize::wxPAPER_A_PLUS },
            { L"paper-b-plus", wxPaperSize::wxPAPER_B_PLUS },
            { L"paper-letter-plus", wxPaperSize::wxPAPER_LETTER_PLUS },
            { L"paper-a4-plus", wxPaperSize::wxPAPER_A4_PLUS },
            { L"paper-a5-transverse", wxPaperSize::wxPAPER_A5_TRANSVERSE },
            { L"paper-b5-transverse", wxPaperSize::wxPAPER_B5_TRANSVERSE },
            { L"paper-a3-extra", wxPaperSize::wxPAPER_A3_EXTRA },
            { L"paper-a5-extra", wxPaperSize::wxPAPER_A5_EXTRA },
            { L"paper-b5-extra", wxPaperSize::wxPAPER_B5_EXTRA },
            { L"paper-a2", wxPaperSize::wxPAPER_A2 },
            { L"paper-a3-transverse", wxPaperSize::wxPAPER_A3_TRANSVERSE },
            { L"paper-a3-extra-transverse", wxPaperSize::wxPAPER_A3_EXTRA_TRANSVERSE },
            { L"paper-dbl-japanese-postcard", wxPaperSize::wxPAPER_DBL_JAPANESE_POSTCARD },
            { L"paper-a6", wxPaperSize::wxPAPER_A6 },
            { L"paper-jenv-kaku2", wxPaperSize::wxPAPER_JENV_KAKU2 },
            { L"paper-jenv-kaku3", wxPaperSize::wxPAPER_JENV_KAKU3 },
            { L"paper-jenv-chou3", wxPaperSize::wxPAPER_JENV_CHOU3 },
            { L"paper-jenv-chou4", wxPaperSize::wxPAPER_JENV_CHOU4 },
            { L"paper-letter-rotated", wxPaperSize::wxPAPER_LETTER_ROTATED },
            { L"paper-a3-rotated", wxPaperSize::wxPAPER_A3_ROTATED },
            { L"paper-a4-rotated", wxPaperSize::wxPAPER_A4_ROTATED },
            { L"paper-a5-rotated", wxPaperSize::wxPAPER_A5_ROTATED },
            { L"paper-b4-jis-rotated", wxPaperSize::wxPAPER_B4_JIS_ROTATED },
            { L"paper-b5-jis-rotated", wxPaperSize::wxPAPER_B5_JIS_ROTATED },
            { L"paper-japanese-postcard-rotated", wxPaperSize::wxPAPER_JAPANESE_POSTCARD_ROTATED },
            { L"paper-dbl-japanese-postcard-rotated",
              wxPaperSize::wxPAPER_DBL_JAPANESE_POSTCARD_ROTATED },
            { L"paper-a6-rotated", wxPaperSize::wxPAPER_A6_ROTATED },
            { L"paper-jenv-kaku2-rotated", wxPaperSize::wxPAPER_JENV_KAKU2_ROTATED },
            { L"paper-jenv-kaku3-rotated", wxPaperSize::wxPAPER_JENV_KAKU3_ROTATED },
            { L"paper-jenv-chou3-rotated", wxPaperSize::wxPAPER_JENV_CHOU3_ROTATED },
            { L"paper-jenv-chou4-rotated", wxPaperSize::wxPAPER_JENV_CHOU4_ROTATED },
            { L"paper-b6-jis", wxPaperSize::wxPAPER_B6_JIS },
            { L"paper-b6-jis-rotated", wxPaperSize::wxPAPER_B6_JIS_ROTATED },
            { L"paper-12x11", wxPaperSize::wxPAPER_12X11 },
            { L"paper-jenv-you4", wxPaperSize::wxPAPER_JENV_YOU4 },
            { L"paper-jenv-you4-rotated", wxPaperSize::wxPAPER_JENV_YOU4_ROTATED },
            { L"paper-p16k", wxPaperSize::wxPAPER_P16K },
            { L"paper-p32k", wxPaperSize::wxPAPER_P32K },
            { L"paper-p32kbig", wxPaperSize::wxPAPER_P32KBIG },
            { L"paper-penv-1", wxPaperSize::wxPAPER_PENV_1 },
            { L"paper-penv-2", wxPaperSize::wxPAPER_PENV_2 },
            { L"paper-penv-3", wxPaperSize::wxPAPER_PENV_3 },
            { L"paper-penv-4", wxPaperSize::wxPAPER_PENV_4 },
            { L"paper-penv-5", wxPaperSize::wxPAPER_PENV_5 },
            { L"paper-penv-6", wxPaperSize::wxPAPER_PENV_6 },
            { L"paper-penv-7", wxPaperSize::wxPAPER_PENV_7 },
            { L"paper-penv-8", wxPaperSize::wxPAPER_PENV_8 },
            { L"paper-penv-9", wxPaperSize::wxPAPER_PENV_9 },
            { L"paper-penv-10", wxPaperSize::wxPAPER_PENV_10 },
            { L"paper-p16k-rotated", wxPaperSize::wxPAPER_P16K_ROTATED },
            { L"paper-p32k-rotated", wxPaperSize::wxPAPER_P32K_ROTATED },
            { L"paper-p32kbig-rotated", wxPaperSize::wxPAPER_P32KBIG_ROTATED },
            { L"paper-penv-1-rotated", wxPaperSize::wxPAPER_PENV_1_ROTATED },
            { L"paper-penv-2-rotated", wxPaperSize::wxPAPER_PENV_2_ROTATED },
            { L"paper-penv-3-rotated", wxPaperSize::wxPAPER_PENV_3_ROTATED },
            { L"paper-penv-4-rotated", wxPaperSize::wxPAPER_PENV_4_ROTATED },
            { L"paper-penv-5-rotated", wxPaperSize::wxPAPER_PENV_5_ROTATED },
            { L"paper-penv-6-rotated", wxPaperSize::wxPAPER_PENV_6_ROTATED },
            { L"paper-penv-7-rotated", wxPaperSize::wxPAPER_PENV_7_ROTATED },
            { L"paper-penv-8-rotated", wxPaperSize::wxPAPER_PENV_8_ROTATED },
            { L"paper-penv-9-rotated", wxPaperSize::wxPAPER_PENV_9_ROTATED },
            { L"paper-penv-10-rotated", wxPaperSize::wxPAPER_PENV_10_ROTATED },
            { L"paper-a0", wxPaperSize::wxPAPER_A0 },
            { L"paper-a1", wxPaperSize::wxPAPER_A1 }
        };
        // NOLINTEND

      public:
        //---------------------------------------------------
        [[nodiscard]]
        static std::shared_ptr<Colors::Schemes::ColorScheme>
        ConvertColorScheme(wxString schemeColor)
            {
            static std::map<std::wstring_view, std::shared_ptr<Colors::Schemes::ColorScheme>>
                colorSchemes = {
                    { L"dusk", std::make_shared<Colors::Schemes::Dusk>() },
                    { L"earthtones", std::make_shared<Colors::Schemes::EarthTones>() },
                    { L"decade1920s", std::make_shared<Colors::Schemes::Decade1920s>() },
                    { L"decade1940s", std::make_shared<Colors::Schemes::Decade1940s>() },
                    { L"decade1950s", std::make_shared<Colors::Schemes::Decade1950s>() },
                    { L"decade1960s", std::make_shared<Colors::Schemes::Decade1960s>() },
                    { L"decade1970s", std::make_shared<Colors::Schemes::Decade1970s>() },
                    { L"decade1980s", std::make_shared<Colors::Schemes::Decade1980s>() },
                    { L"decade1990s", std::make_shared<Colors::Schemes::Decade1990s>() },
                    { L"decade2000s", std::make_shared<Colors::Schemes::Decade2000s>() },
                    { L"october", std::make_shared<Colors::Schemes::October>() },
                    { L"slytherin", std::make_shared<Colors::Schemes::Slytherin>() },
                    { L"campfire", std::make_shared<Colors::Schemes::Campfire>() },
                    { L"coffeeshop", std::make_shared<Colors::Schemes::CoffeeShop>() },
                    { L"arcticchill", std::make_shared<Colors::Schemes::ArcticChill>() },
                    { L"backtoschool", std::make_shared<Colors::Schemes::BackToSchool>() },
                    { L"boxofchocolates", std::make_shared<Colors::Schemes::BoxOfChocolates>() },
                    { L"cosmopolitan", std::make_shared<Colors::Schemes::Cosmopolitan>() },
                    { L"dayandnight", std::make_shared<Colors::Schemes::DayAndNight>() },
                    { L"freshflowers", std::make_shared<Colors::Schemes::FreshFlowers>() },
                    { L"icecream", std::make_shared<Colors::Schemes::IceCream>() },
                    { L"urbanoasis", std::make_shared<Colors::Schemes::UrbanOasis>() },
                    { L"typewriter", std::make_shared<Colors::Schemes::Typewriter>() },
                    { L"tastywaves", std::make_shared<Colors::Schemes::TastyWaves>() },
                    { L"spring", std::make_shared<Colors::Schemes::Spring>() },
                    { L"shabbychic", std::make_shared<Colors::Schemes::ShabbyChic>() },
                    { L"rollingthunder", std::make_shared<Colors::Schemes::RollingThunder>() },
                    { L"producesection", std::make_shared<Colors::Schemes::ProduceSection>() },
                    { L"nautical", std::make_shared<Colors::Schemes::Nautical>() },
                    { L"semesters", std::make_shared<Colors::Schemes::Semesters>() },
                    { L"seasons", std::make_shared<Colors::Schemes::Seasons>() },
                    { L"meadowsunset", std::make_shared<Colors::Schemes::MeadowSunset>() }
                };

            const auto foundPos =
                colorSchemes.find(std::wstring_view(schemeColor.MakeLower().wc_str()));
            return (foundPos != colorSchemes.cend() ? foundPos->second : nullptr);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Icons::IconShape> ConvertIcon(wxString iconStr)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring_view, Icons::IconShape> iconEnums = {
                { L"blank", Icons::IconShape::Blank },
                { L"horizontal-line", Icons::IconShape::HorizontalLine },
                { L"vertical-line", Icons::IconShape::VerticalLine },
                { L"crossed-out", Icons::IconShape::CrossedOut },
                { L"arrow-right", Icons::IconShape::ArrowRight },
                { L"circle", Icons::IconShape::Circle },
                { L"image", Icons::IconShape::Image },
                { L"horizontal-separator", Icons::IconShape::HorizontalSeparator },
                { L"horizontal-arrow-right-separator",
                  Icons::IconShape::HorizontalArrowRightSeparator },
                { L"color-gradient", Icons::IconShape::ColorGradient },
                { L"square", Icons::IconShape::Square },
                { L"triangle-upward", Icons::IconShape::TriangleUpward },
                { L"triangle-downward", Icons::IconShape::TriangleDownward },
                { L"triangle-right", Icons::IconShape::TriangleRight },
                { L"triangle-left", Icons::IconShape::TriangleLeft },
                { L"diamond", Icons::IconShape::Diamond },
                { L"plus", Icons::IconShape::Plus },
                { L"asterisk", Icons::IconShape::Asterisk },
                { L"hexagon", Icons::IconShape::Hexagon },
                { L"box-plot", Icons::IconShape::BoxPlot },
                { L"location-marker", Icons::IconShape::LocationMarker },
                { L"go-road-sign", Icons::IconShape::GoRoadSign },
                { L"warning-road-sign", Icons::IconShape::WarningRoadSign },
                { L"sun", Icons::IconShape::Sun },
                { L"flower", Icons::IconShape::Flower },
                { L"fall-leaf", Icons::IconShape::FallLeaf },
                { L"top-curly-brace", Icons::IconShape::TopCurlyBrace },
                { L"right-curly-brace", Icons::IconShape::RightCurlyBrace },
                { L"bottom-curly-brace", Icons::IconShape::BottomCurlyBrace },
                { L"left-curly-brace", Icons::IconShape::LeftCurlyBrace },
                { L"man", Icons::IconShape::Man },
                { L"woman", Icons::IconShape::Woman },
                { L"business-woman", Icons::IconShape::BusinessWoman },
                { L"chevron-downward", Icons::IconShape::ChevronDownward },
                { L"chevron-upward", Icons::IconShape::ChevronUpward },
                { L"text", Icons::IconShape::Text },
                { L"tack", Icons::IconShape::Tack },
                { L"banner", Icons::IconShape::Banner },
                { L"watercolor-rectangle", Icons::IconShape::WaterColorRectangle },
                { L"thick-watercolor-rectangle", Icons::IconShape::ThickWaterColorRectangle },
                { L"marker-rectangle", Icons::IconShape::MarkerRectangle },
                { L"pencil-rectangle", Icons::IconShape::PencilRectangle },
                { L"graduation-cap", Icons::IconShape::GraduationCap },
                { L"book", Icons::IconShape::Book },
                { L"tire", Icons::IconShape::Tire },
                { L"snowflake", Icons::IconShape::Snowflake },
                { L"newspaper", Icons::IconShape::Newspaper },
                { L"car", Icons::IconShape::Car },
                { L"blackboard", Icons::IconShape::Blackboard },
                { L"clock", Icons::IconShape::Clock },
                { L"ruler", Icons::IconShape::Ruler },
                { L"ivbag", Icons::IconShape::IVBag },
                { L"cold-thermometer", Icons::IconShape::ColdThermometer },
                { L"hot-thermometer", Icons::IconShape::HotThermometer },
                { L"apple", Icons::IconShape::Apple },
                { L"granny-smith-apple", Icons::IconShape::GrannySmithApple },
                { L"heart", Icons::IconShape::Heart },
                { L"immaculate-heart", Icons::IconShape::ImmaculateHeart },
                { L"flame", Icons::IconShape::Flame },
                { L"office", Icons::IconShape::Office },
                { L"factory", Icons::IconShape::Factory },
                { L"house", Icons::IconShape::House },
                { L"barn", Icons::IconShape::Barn },
                { L"farm", Icons::IconShape::Farm },
                { L"hundred-dollar-bill", Icons::IconShape::HundredDollarBill },
                { L"monitor", Icons::IconShape::Monitor },
                { L"sword", Icons::IconShape::Sword },
                { L"immaculate-heart-with-sword", Icons::IconShape::ImmaculateHeartWithSword },
                { L"crescent-top", Icons::IconShape::CrescentTop },
                { L"crescent-bottom", Icons::IconShape::CrescentBottom },
                { L"crescent-right", Icons::IconShape::CrescentRight },
                { L"sunflower", Icons::IconShape::Sunflower },
                { L"curving-road", Icons::IconShape::CurvingRoad },
                { L"pumpkin", Icons::IconShape::Pumpkin },
                { L"jack-o-lantern", Icons::IconShape::JackOLantern },
                { L"number-range", Icons::IconShape::NumberRange },
                { L"cheese-pizza", Icons::IconShape::CheesePizza },
                { L"pepperoni-pizza", Icons::IconShape::PepperoniPizza },
                { L"hawaiian-pizza", Icons::IconShape::HawaiianPizza },
                { L"chocolate-chip-cookie", Icons::IconShape::ChocolateChipCookie },
                { L"coffee-shop-cup", Icons::IconShape::CoffeeShopCup },
                { L"pill", Icons::IconShape::Pill },
                { L"tractor", Icons::IconShape::Tractor }
            };

            const auto foundPos = iconEnums.find(std::wstring_view(iconStr.MakeLower().wc_str()));
            return (foundPos != iconEnums.cend() ?
                        std::optional<Icons::IconShape>(foundPos->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxPaperSize> ConvertPaperSize(const wxString& value)
            {
            const auto foundValue = m_paperSizeValues.find(value.Lower().ToStdWstring());
            return ((foundValue != m_paperSizeValues.cend()) ?
                        std::optional<wxPaperSize>(foundValue->second) :
                        std::nullopt);
            }

        /// @brief Converts a wxPaperSize enum to its JSON string representation.
        /// @param value The paper size enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertPaperSizeToString(wxPaperSize value)
            {
            for (const auto& [name, size] : m_paperSizeValues)
                {
                if (size == value)
                    {
                    return wxString{ name };
                    }
                }
            return std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<LabelPlacement> ConvertLabelPlacement(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, LabelPlacement> labelPlacementValues = {
                { L"next-to-parent", LabelPlacement::NextToParent },
                { L"flush", LabelPlacement::Flush }
            };

            const auto foundValue = labelPlacementValues.find(value.Lower().ToStdWstring());
            return ((foundValue != labelPlacementValues.cend()) ?
                        std::optional<LabelPlacement>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<TextAlignment> ConvertTextAlignment(const wxString& value)
            {
            static const std::map<std::wstring, TextAlignment> textAlignValues = {
                { L"flush-left", TextAlignment::FlushLeft },
                { L"flush-right", TextAlignment::FlushRight },
                { L"ragged-right", TextAlignment::RaggedRight },
                { L"ragged-left", TextAlignment::RaggedLeft },
                { L"centered", TextAlignment::Centered },
                { L"justified", TextAlignment::Justified },
                { L"justified-at-character", TextAlignment::JustifiedAtCharacter },
                { L"justified-at-word", TextAlignment::JustifiedAtWord }
            };

            const auto foundValue = textAlignValues.find(value.Lower().ToStdWstring());
            return ((foundValue != textAlignValues.cend()) ?
                        std::optional<TextAlignment>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<GraphColumnHeader> ConvertGraphColumnHeader(const wxString& value)
            {
            static const std::map<std::wstring, GraphColumnHeader> graphColumnHeader = {
                { L"as-header", GraphColumnHeader::AsHeader },
                { L"as-footer", GraphColumnHeader::AsFooter },
                { L"no-display", GraphColumnHeader::NoDisplay }
            };

            const auto foundValue = graphColumnHeader.find(value.Lower().ToStdWstring());
            return ((foundValue != graphColumnHeader.cend()) ?
                        std::optional<GraphColumnHeader>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<ReferenceLabelPlacement>
        ConvertReferenceLabelPlacement(const wxString& value)
            {
            static const std::map<std::wstring, ReferenceLabelPlacement> referenceLabelPlacement = {
                { L"opposite-axis", ReferenceLabelPlacement::OppositeAxis },
                { L"legend", ReferenceLabelPlacement::Legend }
            };

            const auto foundValue = referenceLabelPlacement.find(value.Lower().ToStdWstring());
            return ((foundValue != referenceLabelPlacement.cend()) ?
                        std::optional<ReferenceLabelPlacement>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<FlowShape> ConvertFlowShape(const wxString& value)
            {
            static const std::map<std::wstring, FlowShape> flowShapeValues = {
                { L"curvy", FlowShape::Curvy }, { L"jagged", FlowShape::Jagged }
            };

            const auto foundValue = flowShapeValues.find(value.Lower().ToStdWstring());
            return ((foundValue != flowShapeValues.cend()) ?
                        std::optional<FlowShape>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxBrushStyle> ConvertBrushStyle(const wxString& value)
            {
            static const std::map<std::wstring, wxBrushStyle> styleValues = {
                { L"backwards-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_BDIAGONAL_HATCH },
                { L"forward-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_FDIAGONAL_HATCH },
                { L"cross-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_CROSSDIAG_HATCH },
                { L"solid", wxBrushStyle::wxBRUSHSTYLE_SOLID },
                { L"cross-hatch", wxBrushStyle::wxBRUSHSTYLE_CROSS_HATCH },
                { L"horizontal-hatch", wxBrushStyle::wxBRUSHSTYLE_HORIZONTAL_HATCH },
                { L"vertical-hatch", wxBrushStyle::wxBRUSHSTYLE_VERTICAL_HATCH }
            };

            const auto foundValue = styleValues.find(value.Lower().ToStdWstring());
            return ((foundValue != styleValues.cend()) ?
                        std::optional<wxBrushStyle>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<DateInterval> ConvertDateInterval(const wxString& value)
            {
            static const std::map<std::wstring, DateInterval> dateValues = {
                { L"daily", DateInterval::Daily },
                { L"fiscal-quarterly", DateInterval::FiscalQuarterly },
                { L"monthly", DateInterval::Monthly },
                { L"weekly", DateInterval::Weekly }
            };

            const auto foundValue = dateValues.find(value.Lower().ToStdWstring());
            return ((foundValue != dateValues.cend()) ?
                        std::optional<DateInterval>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<TableCellFormat> ConvertTableCellFormat(const wxString& value)
            {
            static const std::map<std::wstring, TableCellFormat> formatValues = {
                { L"accounting", TableCellFormat::Accounting },
                { L"general", TableCellFormat::General },
                { L"percent", TableCellFormat::Percent },
                { L"percent-changed", TableCellFormat::PercentChange }
            };

            const auto foundValue = formatValues.find(value.Lower().ToStdWstring());
            return ((foundValue != formatValues.cend()) ?
                        std::optional<TableCellFormat>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<FiscalYear> ConvertFiscalYear(const wxString& value)
            {
            static const std::map<std::wstring, FiscalYear> fyValues = {
                { L"education", FiscalYear::Education }, { L"us-business", FiscalYear::USBusiness }
            };

            const auto foundValue = fyValues.find(value.Lower().ToStdWstring());
            return ((foundValue != fyValues.cend()) ?
                        std::optional<FiscalYear>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::GanttChart::TaskLabelDisplay>
        ConvertTaskLabelDisplay(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::GanttChart::TaskLabelDisplay>
                taskLabelDisplays = {
                    { L"days", Graphs::GanttChart::TaskLabelDisplay::Days },
                    { L"description", Graphs::GanttChart::TaskLabelDisplay::Description },
                    { L"description-and-days",
                      Graphs::GanttChart::TaskLabelDisplay::DescriptionAndDays },
                    { L"no-display", Graphs::GanttChart::TaskLabelDisplay::NoDisplay },
                    { L"resource", Graphs::GanttChart::TaskLabelDisplay::Resource },
                    { L"resource-and-days", Graphs::GanttChart::TaskLabelDisplay::ResourceAndDays },
                    { L"resource-and-description",
                      Graphs::GanttChart::TaskLabelDisplay::ResourceAndDescription },
                    { L"resource-description-and-days",
                      Graphs::GanttChart::TaskLabelDisplay::ResourceDescriptionAndDays }
                };

            const auto foundValue = taskLabelDisplays.find(value.Lower().ToStdWstring());
            return ((foundValue != taskLabelDisplays.cend()) ?
                        std::optional<Graphs::GanttChart::TaskLabelDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::CandlestickPlot::PlotType>
        ConvertCandlestickPlotType(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::CandlestickPlot::PlotType> candleTypes = {
                { L"candlestick", Graphs::CandlestickPlot::PlotType::Candlestick },
                { L"ohlc", Graphs::CandlestickPlot::PlotType::Ohlc }
            };

            const auto foundValue = candleTypes.find(value.Lower().ToStdWstring());
            return ((foundValue != candleTypes.cend()) ?
                        std::optional<Graphs::CandlestickPlot::PlotType>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::LikertChart::LikertSurveyQuestionFormat>
        ConvertLikertSurveyQuestionFormat(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::LikertChart::LikertSurveyQuestionFormat>
                surveyTypes = {
                    { L"two-point", Graphs::LikertChart::LikertSurveyQuestionFormat::TwoPoint },
                    { L"two-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::TwoPointCategorized },
                    { L"three-point", Graphs::LikertChart::LikertSurveyQuestionFormat::ThreePoint },
                    { L"threepoint-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::ThreePointCategorized },
                    { L"four-point", Graphs::LikertChart::LikertSurveyQuestionFormat::FourPoint },
                    { L"four-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::FourPointCategorized },
                    { L"five-point", Graphs::LikertChart::LikertSurveyQuestionFormat::FivePoint },
                    { L"five-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::FivePointCategorized },
                    { L"six-point", Graphs::LikertChart::LikertSurveyQuestionFormat::SixPoint },
                    { L"six-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::SixPointCategorized },
                    { L"seven-point", Graphs::LikertChart::LikertSurveyQuestionFormat::SevenPoint },
                    { L"seven-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::SevenPointCategorized }
                };

            const auto foundValue = surveyTypes.find(value.Lower().ToStdWstring());
            return ((foundValue != surveyTypes.cend()) ?
                        std::optional<Graphs::LikertChart::LikertSurveyQuestionFormat>(
                            foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<BoxEffect> ConvertBoxEffect(const wxString& value)
            {
            static const std::map<std::wstring_view, BoxEffect> boxEffects = {
                { L"common-image", BoxEffect::CommonImage },
                { L"image", BoxEffect::Image },
                { L"fade-from-bottom-to-top", BoxEffect::FadeFromBottomToTop },
                { L"fade-from-left-to-right", BoxEffect::FadeFromLeftToRight },
                { L"fade-from-right-to-left", BoxEffect::FadeFromRightToLeft },
                { L"fade-from-top-to-bottom", BoxEffect::FadeFromTopToBottom },
                { L"glassy", BoxEffect::Glassy },
                { L"solid", BoxEffect::Solid },
                { L"stipple-image", BoxEffect::StippleImage },
                { L"stipple-shape", BoxEffect::StippleShape },
                { L"watercolor", BoxEffect::WaterColor },
                { L"marker", BoxEffect::Marker },
                { L"pencil", BoxEffect::Pencil }
            };

            const auto foundValue = boxEffects.find(value.Lower().ToStdWstring());
            return ((foundValue != boxEffects.cend()) ?
                        std::optional<BoxEffect>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<PieSliceEffect> ConvertPieSliceEffect(const wxString& value)
            {
            static const std::map<std::wstring_view, PieSliceEffect> sliceEffects = {
                { L"image", PieSliceEffect::Image }, { L"solid", PieSliceEffect::Solid }
            };

            const auto foundValue = sliceEffects.find(value.Lower().ToStdWstring());
            return ((foundValue != sliceEffects.cend()) ?
                        std::optional<PieSliceEffect>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<PieStyle> ConvertPieStyle(const wxString& value)
            {
            static const std::map<std::wstring_view, PieStyle> sliceEffects = {
                { L"none", PieStyle::None },
                { L"clockface", PieStyle::Clockface },
                { L"cheese-pizza", PieStyle::CheesePizza },
                { L"pepperoni-pizza", PieStyle::PepperoniPizza },
                { L"hawaiian-pizza", PieStyle::HawaiianPizza },
                { L"coffee-ring", PieStyle::CoffeeRing },
                { L"venus", PieStyle::Venus },
                { L"mars", PieStyle::Mars },
                { L"chocolate-chip-cookie", PieStyle::ChocolateChipCookie }
            };

            const auto foundValue = sliceEffects.find(value.Lower().ToStdWstring());
            return ((foundValue != sliceEffects.cend()) ?
                        std::optional<PieStyle>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Perimeter> ConvertPerimeter(const wxString& value)
            {
            static const std::map<std::wstring_view, Perimeter> peris = {
                { L"inner", Perimeter::Inner }, { L"outer", Perimeter::Outer }
            };

            const auto foundValue = peris.find(value.Lower().ToStdWstring());
            return ((foundValue != peris.cend()) ? std::optional<Perimeter>(foundValue->second) :
                                                   std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Histogram::BinningMethod>
        ConvertBinningMethod(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::Histogram::BinningMethod> binMethods = {
                { L"bin-by-integer-range", Graphs::Histogram::BinningMethod::BinByIntegerRange },
                { L"bin-by-range", Graphs::Histogram::BinningMethod::BinByRange },
                { L"bin-unique-values", Graphs::Histogram::BinningMethod::BinUniqueValues }
            };

            const auto foundValue = binMethods.find(value.Lower().ToStdWstring());
            return ((foundValue != binMethods.cend()) ?
                        std::optional<Graphs::Histogram::BinningMethod>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Histogram::IntervalDisplay>
        ConvertIntervalDisplay(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::Histogram::IntervalDisplay> binIntervals = {
                { L"cutpoints", Graphs::Histogram::IntervalDisplay::Cutpoints },
                { L"midpoints", Graphs::Histogram::IntervalDisplay::Midpoints }
            };

            const auto foundValue = binIntervals.find(value.Lower().ToStdWstring());
            return ((foundValue != binIntervals.cend()) ?
                        std::optional<Graphs::Histogram::IntervalDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<RoundingMethod> ConvertRoundingMethod(const wxString& value)
            {
            static const std::map<std::wstring, RoundingMethod> roundingMethods = {
                { L"no-rounding", RoundingMethod::NoRounding },
                { L"round", RoundingMethod::Round },
                { L"round-down", RoundingMethod::RoundDown },
                { L"round-up", RoundingMethod::RoundUp }
            };

            const auto foundValue = roundingMethods.find(value.Lower().ToStdWstring());
            return ((foundValue != roundingMethods.cend()) ?
                        std::optional<RoundingMethod>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<BinLabelDisplay> ConvertBinLabelDisplay(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, BinLabelDisplay> bDisplayValues = {
                { L"percentage", BinLabelDisplay::BinPercentage },
                { L"value", BinLabelDisplay::BinValue },
                { L"value-and-percentage", BinLabelDisplay::BinValueAndPercentage },
                { L"no-display", BinLabelDisplay::NoDisplay },
                { L"bin-name", BinLabelDisplay::BinName },
                { L"bin-name-and-value", BinLabelDisplay::BinNameAndValue },
                { L"bin-name-and-percentage", BinLabelDisplay::BinNameAndPercentage }
            };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<BinLabelDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<NumberDisplay> ConvertNumberDisplay(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, NumberDisplay> bDisplayValues = {
                { L"percentage", NumberDisplay::Percentage },
                { L"value", NumberDisplay::Value },
                { L"currency", NumberDisplay::Currency },
                { L"value-simple", NumberDisplay::ValueSimple }
            };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<NumberDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Roadmap::LaneSeparatorStyle>
        ConvertLaneSeparatorStyle(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::Roadmap::LaneSeparatorStyle>
                bDisplayValues = {
                    { L"single-line", Graphs::Roadmap::LaneSeparatorStyle::SingleLine },
                    { L"double-line", Graphs::Roadmap::LaneSeparatorStyle::DoubleLine },
                    { L"no-display", Graphs::Roadmap::LaneSeparatorStyle::NoDisplay }
                };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<Graphs::Roadmap::LaneSeparatorStyle>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Roadmap::RoadStopTheme>
        ConvertRoadStopTheme(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::Roadmap::RoadStopTheme> bDisplayValues = {
                { L"location-markers", Graphs::Roadmap::RoadStopTheme::LocationMarkers },
                { L"road-signs", Graphs::Roadmap::RoadStopTheme::RoadSigns }
            };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<Graphs::Roadmap::RoadStopTheme>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Roadmap::MarkerLabelDisplay>
        ConvertMarkerLabelDisplay(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::Roadmap::MarkerLabelDisplay>
                bDisplayValues = { { L"name", Graphs::Roadmap::MarkerLabelDisplay::Name },
                                   { L"name-and-absolute-value",
                                     Graphs::Roadmap::MarkerLabelDisplay::NameAndAbsoluteValue },
                                   { L"name-and-value",
                                     Graphs::Roadmap::MarkerLabelDisplay::NameAndValue } };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<Graphs::Roadmap::MarkerLabelDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<AxisType> ConvertAxisType(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, AxisType> axisValues = {
                { L"bottom-x", AxisType::BottomXAxis },
                { L"top-x", AxisType::TopXAxis },
                { L"left-y", AxisType::LeftYAxis },
                { L"right-y", AxisType::RightYAxis }
            };

            const auto foundValue = axisValues.find(value.Lower().ToStdWstring());
            return ((foundValue != axisValues.cend()) ?
                        std::optional<AxisType>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::BarChart::BarShape> ConvertBarShape(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::BarChart::BarShape> barShapeValues = {
                { L"rectangle", Graphs::BarChart::BarShape::Rectangle },
                { L"arrow", Graphs::BarChart::BarShape::Arrow },
                { L"reverse-arrow", Graphs::BarChart::BarShape::ReverseArrow }
            };

            const auto foundValue = barShapeValues.find(value.Lower().ToStdWstring());
            return ((foundValue != barShapeValues.cend()) ?
                        std::optional<Graphs::BarChart::BarShape>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertBarShapeToString(Graphs::BarChart::BarShape value)
            {
            static const std::map<Graphs::BarChart::BarShape, wxString> values = {
                { Graphs::BarChart::BarShape::Rectangle, L"rectangle" },
                { Graphs::BarChart::BarShape::Arrow, L"arrow" },
                { Graphs::BarChart::BarShape::ReverseArrow, L"reverse-arrow" }
            };

            const auto foundValue = values.find(value);
            return ((foundValue != values.cend()) ? std::optional<wxString>(foundValue->second) :
                                                    std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<ImageEffect> ConvertImageEffect(const wxString& value)
            {
            static const std::map<std::wstring, ImageEffect> imgEffectValues = {
                { L"blur-horizontal", ImageEffect::BlurHorizontal },
                { L"blur-vertical", ImageEffect::BlurVertical },
                { L"frosted-glass", ImageEffect::FrostedGlass },
                { L"gray-scale", ImageEffect::Grayscale },
                { L"oil-painting", ImageEffect::OilPainting },
                { L"sepia", ImageEffect::Sepia },
                { L"no-effect", ImageEffect::NoEffect }
            };

            const auto foundValue = imgEffectValues.find(value.Lower().ToStdWstring());
            return ((foundValue != imgEffectValues.cend()) ?
                        std::optional<ImageEffect>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Gender> ConvertGender(const wxString& value)
            {
            static const std::map<std::wstring, Gender> genderValues = {
                { L"female", Gender::Female }, { L"male", Gender::Male }
            };

            const auto foundValue = genderValues.find(value.Lower().ToStdWstring());
            return ((foundValue != genderValues.cend()) ?
                        std::optional<Gender>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<HairStyle> ConvertHairStyle(const wxString& value)
            {
            static const std::map<std::wstring, HairStyle> hairStyleValues = {
                { L"bald", HairStyle::Bald },
                { L"bob", HairStyle::Bob },
                { L"pixie", HairStyle::Pixie },
                { L"bun", HairStyle::Bun },
                { L"long-straight", HairStyle::LongStraight },
                { L"high-top-fade", HairStyle::HighTopFade }
            };

            const auto foundValue = hairStyleValues.find(value.Lower().ToStdWstring());
            return ((foundValue != hairStyleValues.cend()) ?
                        std::optional<HairStyle>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<FacialHair> ConvertFacialHair(const wxString& value)
            {
            static const std::map<std::wstring, FacialHair> facialHairValues = {
                { L"clean-shaven", FacialHair::CleanShaven },
                { L"five-o-clock-shadow", FacialHair::FiveOClockShadow }
            };

            const auto foundValue = facialHairValues.find(value.Lower().ToStdWstring());
            return ((foundValue != facialHairValues.cend()) ?
                        std::optional<FacialHair>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxPenStyle> ConvertPenStyle(const wxString& value)
            {
            static const std::map<std::wstring, wxPenStyle> styleValues = {
                { L"dot", wxPenStyle::wxPENSTYLE_DOT },
                { L"dot-dash", wxPenStyle::wxPENSTYLE_DOT_DASH },
                { L"long-dash", wxPenStyle::wxPENSTYLE_LONG_DASH },
                { L"short-dash", wxPenStyle::wxPENSTYLE_SHORT_DASH },
                { L"solid", wxPenStyle::wxPENSTYLE_SOLID },
                { L"cross-hatch", wxPenStyle::wxPENSTYLE_CROSS_HATCH },
                { L"horizontal-hatch", wxPenStyle::wxPENSTYLE_HORIZONTAL_HATCH },
                { L"vertical-hatch", wxPenStyle::wxPENSTYLE_VERTICAL_HATCH }
            };

            const auto foundValue = styleValues.find(value.Lower().ToStdWstring());
            return (foundValue != styleValues.cend()) ?
                       std::optional<wxPenStyle>{ foundValue->second } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Anchoring> ConvertAnchoring(const wxString& value)
            {
            static const std::map<std::wstring, Anchoring> anchoringValues = {
                { L"bottom-left-corner", Anchoring::BottomLeftCorner },
                { L"bottom-right-corner", Anchoring::BottomRightCorner },
                { L"center", Anchoring::Center },
                { L"top-left-corner", Anchoring::TopLeftCorner },
                { L"top-right-corner", Anchoring::TopRightCorner }
            };

            const auto foundValue = anchoringValues.find(value.Lower().ToStdWstring());
            return (foundValue != anchoringValues.cend()) ?
                       std::optional<Anchoring>{ foundValue->second } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<GraphItems::Axis::TickMark::DisplayType>
        ConvertTickMarkDisplay(const wxString& value)
            {
            static const std::map<std::wstring, GraphItems::Axis::TickMark::DisplayType>
                tickmarkValues = { { L"inner", GraphItems::Axis::TickMark::DisplayType::Inner },
                                   { L"outer", GraphItems::Axis::TickMark::DisplayType::Outer },
                                   { L"crossed", GraphItems::Axis::TickMark::DisplayType::Crossed },
                                   { L"no-display",
                                     GraphItems::Axis::TickMark::DisplayType::NoDisplay } };

            const auto foundValue = tickmarkValues.find(value.Lower().ToStdWstring());
            return (foundValue != tickmarkValues.cend()) ?
                       std::optional<GraphItems::Axis::TickMark::DisplayType>{
                           foundValue->second
                       } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<AxisLabelDisplay> ConvertAxisLabelDisplay(const wxString& value)
            {
            static const std::map<std::wstring, AxisLabelDisplay> labelDisplayValues = {
                { L"custom-labels-or-values", AxisLabelDisplay::DisplayCustomLabelsOrValues },
                { L"only-custom-labels", AxisLabelDisplay::DisplayOnlyCustomLabels },
                { L"custom-labels-and-values", AxisLabelDisplay::DisplayCustomLabelsAndValues },
                { L"no-display", AxisLabelDisplay::NoDisplay },
                { L"values", AxisLabelDisplay::DisplayValues }
            };

            const auto foundValue = labelDisplayValues.find(value.Lower().ToStdWstring());
            return (foundValue != labelDisplayValues.cend()) ?
                       std::optional<AxisLabelDisplay>{ foundValue->second } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<BracketLineStyle> ConvertBracketLineStyle(const wxString& value)
            {
            static const std::map<std::wstring, BracketLineStyle> bracketLineValues = {
                { L"arrow", BracketLineStyle::Arrow },
                { L"lines", BracketLineStyle::Lines },
                { L"reverse-arrow", BracketLineStyle::ReverseArrow },
                { L"curly-braces", BracketLineStyle::CurlyBraces },
                { L"no-connection-lines", BracketLineStyle::NoConnectionLines }
            };

            const auto foundValue = bracketLineValues.find(value.Lower().ToStdWstring());
            return (foundValue != bracketLineValues.cend()) ?
                       std::optional<BracketLineStyle>{ foundValue->second } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<LineStyle> ConvertLineStyle(const wxString& value)
            {
            static const std::map<std::wstring, LineStyle> lineStyleValues = {
                { L"arrows", LineStyle::Arrows },
                { L"lines", LineStyle::Lines },
                { L"spline", LineStyle::Spline },
                { L"pencil", LineStyle::Pencil }
            };

            const auto foundValue = lineStyleValues.find(value.Lower().ToStdWstring());
            return (foundValue != lineStyleValues.cend()) ?
                       std::optional<LineStyle>{ foundValue->second } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<ResizeMethod> ConvertResizeMethod(const wxString& value)
            {
            static const std::map<std::wstring, ResizeMethod> resizeValues = {
                { L"downscale-only", ResizeMethod::DownscaleOnly },
                { L"downscale-or-upscale", ResizeMethod::DownscaleOrUpscale },
                { L"upscale-only", ResizeMethod::UpscaleOnly },
                { L"no-resize", ResizeMethod::NoResize }
            };

            const auto foundValue = resizeValues.find(value.Lower().ToStdWstring());
            return (foundValue != resizeValues.cend()) ?
                       std::optional<ResizeMethod>{ foundValue->second } :
                       std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<ReferenceAreaStyle> ConvertReferenceAreaStyle(const wxString& value)
            {
            static const std::map<std::wstring, ReferenceAreaStyle> refAreaValues = {
                { L"fade-from-left-to-right", ReferenceAreaStyle::FadeFromLeftToRight },
                { L"fade-from-right-to-left", ReferenceAreaStyle::FadeFromRightToLeft },
                { L"solid", ReferenceAreaStyle::Solid }
            };

            const auto foundValue = refAreaValues.find(value.Lower().ToStdWstring());
            return (foundValue != refAreaValues.cend()) ?
                       std::optional<ReferenceAreaStyle>{ foundValue->second } :
                       std::nullopt;
            }

        // reverse converters (enum → string)
        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertAnchoringToString(Anchoring value)
            {
            static const std::map<Anchoring, wxString> values = {
                { Anchoring::BottomLeftCorner, L"bottom-left-corner" },
                { Anchoring::BottomRightCorner, L"bottom-right-corner" },
                { Anchoring::Center, L"center" },
                { Anchoring::TopLeftCorner, L"top-left-corner" },
                { Anchoring::TopRightCorner, L"top-right-corner" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString>
        ConvertPageHorizontalAlignmentToString(PageHorizontalAlignment value)
            {
            static const std::map<PageHorizontalAlignment, wxString> values = {
                { PageHorizontalAlignment::LeftAligned, L"left-aligned" },
                { PageHorizontalAlignment::RightAligned, L"right-aligned" },
                { PageHorizontalAlignment::Centered, L"centered" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString>
        ConvertPageVerticalAlignmentToString(PageVerticalAlignment value)
            {
            static const std::map<PageVerticalAlignment, wxString> values = {
                { PageVerticalAlignment::TopAligned, L"top-aligned" },
                { PageVerticalAlignment::BottomAligned, L"bottom-aligned" },
                { PageVerticalAlignment::Centered, L"centered" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertRelativeAlignmentToString(RelativeAlignment value)
            {
            static const std::map<RelativeAlignment, wxString> values = {
                { RelativeAlignment::FlushLeft, L"flush-left" },
                { RelativeAlignment::FlushRight, L"flush-right" },
                { RelativeAlignment::FlushTop, L"flush-top" },
                { RelativeAlignment::FlushBottom, L"flush-bottom" },
                { RelativeAlignment::Centered, L"centered" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertPenStyleToString(wxPenStyle value)
            {
            static const std::map<wxPenStyle, wxString> values = {
                { wxPENSTYLE_DOT, L"dot" },
                { wxPENSTYLE_DOT_DASH, L"dot-dash" },
                { wxPENSTYLE_LONG_DASH, L"long-dash" },
                { wxPENSTYLE_SHORT_DASH, L"short-dash" },
                { wxPENSTYLE_SOLID, L"solid" },
                { wxPENSTYLE_CROSS_HATCH, L"cross-hatch" },
                { wxPENSTYLE_HORIZONTAL_HATCH, L"horizontal-hatch" },
                { wxPENSTYLE_VERTICAL_HATCH, L"vertical-hatch" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertBrushStyleToString(wxBrushStyle value)
            {
            static const std::map<wxBrushStyle, wxString> values = {
                { wxBRUSHSTYLE_BDIAGONAL_HATCH, L"backwards-diagonal-hatch" },
                { wxBRUSHSTYLE_FDIAGONAL_HATCH, L"forward-diagonal-hatch" },
                { wxBRUSHSTYLE_CROSSDIAG_HATCH, L"cross-diagonal-hatch" },
                { wxBRUSHSTYLE_SOLID, L"solid" },
                { wxBRUSHSTYLE_CROSS_HATCH, L"cross-hatch" },
                { wxBRUSHSTYLE_HORIZONTAL_HATCH, L"horizontal-hatch" },
                { wxBRUSHSTYLE_VERTICAL_HATCH, L"vertical-hatch" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertTextAlignmentToString(TextAlignment value)
            {
            static const std::map<TextAlignment, wxString> values = {
                { TextAlignment::FlushLeft, L"flush-left" },
                { TextAlignment::FlushRight, L"flush-right" },
                { TextAlignment::RaggedRight, L"ragged-right" },
                { TextAlignment::RaggedLeft, L"ragged-left" },
                { TextAlignment::Centered, L"centered" },
                { TextAlignment::Justified, L"justified" },
                { TextAlignment::JustifiedAtCharacter, L"justified-at-character" },
                { TextAlignment::JustifiedAtWord, L"justified-at-word" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertIconToString(Icons::IconShape value)
            {
            static const std::map<Icons::IconShape, wxString> values = {
                { Icons::IconShape::Blank, L"blank" },
                { Icons::IconShape::HorizontalLine, L"horizontal-line" },
                { Icons::IconShape::VerticalLine, L"vertical-line" },
                { Icons::IconShape::CrossedOut, L"crossed-out" },
                { Icons::IconShape::ArrowRight, L"arrow-right" },
                { Icons::IconShape::Circle, L"circle" },
                { Icons::IconShape::Image, L"image" },
                { Icons::IconShape::HorizontalSeparator, L"horizontal-separator" },
                { Icons::IconShape::HorizontalArrowRightSeparator,
                  L"horizontal-arrow-right-separator" },
                { Icons::IconShape::ColorGradient, L"color-gradient" },
                { Icons::IconShape::Square, L"square" },
                { Icons::IconShape::TriangleUpward, L"triangle-upward" },
                { Icons::IconShape::TriangleDownward, L"triangle-downward" },
                { Icons::IconShape::TriangleRight, L"triangle-right" },
                { Icons::IconShape::TriangleLeft, L"triangle-left" },
                { Icons::IconShape::Diamond, L"diamond" },
                { Icons::IconShape::Plus, L"plus" },
                { Icons::IconShape::Asterisk, L"asterisk" },
                { Icons::IconShape::Hexagon, L"hexagon" },
                { Icons::IconShape::BoxPlot, L"box-plot" },
                { Icons::IconShape::LocationMarker, L"location-marker" },
                { Icons::IconShape::GoRoadSign, L"go-road-sign" },
                { Icons::IconShape::WarningRoadSign, L"warning-road-sign" },
                { Icons::IconShape::Sun, L"sun" },
                { Icons::IconShape::Flower, L"flower" },
                { Icons::IconShape::Sunflower, L"sunflower" },
                { Icons::IconShape::FallLeaf, L"fall-leaf" },
                { Icons::IconShape::TopCurlyBrace, L"top-curly-brace" },
                { Icons::IconShape::RightCurlyBrace, L"right-curly-brace" },
                { Icons::IconShape::BottomCurlyBrace, L"bottom-curly-brace" },
                { Icons::IconShape::LeftCurlyBrace, L"left-curly-brace" },
                { Icons::IconShape::Man, L"man" },
                { Icons::IconShape::Woman, L"woman" },
                { Icons::IconShape::BusinessWoman, L"business-woman" },
                { Icons::IconShape::ChevronDownward, L"chevron-downward" },
                { Icons::IconShape::ChevronUpward, L"chevron-upward" },
                { Icons::IconShape::Text, L"text" },
                { Icons::IconShape::Tack, L"tack" },
                { Icons::IconShape::Banner, L"banner" },
                { Icons::IconShape::WaterColorRectangle, L"watercolor-rectangle" },
                { Icons::IconShape::ThickWaterColorRectangle, L"thick-watercolor-rectangle" },
                { Icons::IconShape::MarkerRectangle, L"marker-rectangle" },
                { Icons::IconShape::PencilRectangle, L"pencil-rectangle" },
                { Icons::IconShape::GraduationCap, L"graduation-cap" },
                { Icons::IconShape::Book, L"book" },
                { Icons::IconShape::Tire, L"tire" },
                { Icons::IconShape::Snowflake, L"snowflake" },
                { Icons::IconShape::Newspaper, L"newspaper" },
                { Icons::IconShape::Car, L"car" },
                { Icons::IconShape::Blackboard, L"blackboard" },
                { Icons::IconShape::Clock, L"clock" },
                { Icons::IconShape::Ruler, L"ruler" },
                { Icons::IconShape::IVBag, L"ivbag" },
                { Icons::IconShape::ColdThermometer, L"cold-thermometer" },
                { Icons::IconShape::HotThermometer, L"hot-thermometer" },
                { Icons::IconShape::Apple, L"apple" },
                { Icons::IconShape::GrannySmithApple, L"granny-smith-apple" },
                { Icons::IconShape::Flame, L"flame" },
                { Icons::IconShape::Heart, L"heart" },
                { Icons::IconShape::ImmaculateHeart, L"immaculate-heart" },
                { Icons::IconShape::ImmaculateHeartWithSword, L"immaculate-heart-with-sword" },
                { Icons::IconShape::Office, L"office" },
                { Icons::IconShape::Factory, L"factory" },
                { Icons::IconShape::House, L"house" },
                { Icons::IconShape::Barn, L"barn" },
                { Icons::IconShape::Farm, L"farm" },
                { Icons::IconShape::HundredDollarBill, L"hundred-dollar-bill" },
                { Icons::IconShape::Monitor, L"monitor" },
                { Icons::IconShape::Sword, L"sword" },
                { Icons::IconShape::CrescentTop, L"crescent-top" },
                { Icons::IconShape::CrescentBottom, L"crescent-bottom" },
                { Icons::IconShape::CrescentRight, L"crescent-right" },
                { Icons::IconShape::CurvingRoad, L"curving-road" },
                { Icons::IconShape::Pumpkin, L"pumpkin" },
                { Icons::IconShape::JackOLantern, L"jack-o-lantern" },
                { Icons::IconShape::NumberRange, L"number-range" },
                { Icons::IconShape::CheesePizza, L"cheese-pizza" },
                { Icons::IconShape::PepperoniPizza, L"pepperoni-pizza" },
                { Icons::IconShape::HawaiianPizza, L"hawaiian-pizza" },
                { Icons::IconShape::ChocolateChipCookie, L"chocolate-chip-cookie" },
                { Icons::IconShape::CoffeeShopCup, L"coffee-shop-cup" },
                { Icons::IconShape::Pill, L"pill" },
                { Icons::IconShape::Tractor, L"tractor" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString>
        ConvertTickMarkDisplayToString(GraphItems::Axis::TickMark::DisplayType value)
            {
            static const std::map<GraphItems::Axis::TickMark::DisplayType, wxString> values = {
                { GraphItems::Axis::TickMark::DisplayType::Inner, L"inner" },
                { GraphItems::Axis::TickMark::DisplayType::Outer, L"outer" },
                { GraphItems::Axis::TickMark::DisplayType::Crossed, L"crossed" },
                { GraphItems::Axis::TickMark::DisplayType::NoDisplay, L"no-display" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertAxisLabelDisplayToString(AxisLabelDisplay value)
            {
            static const std::map<AxisLabelDisplay, wxString> values = {
                { AxisLabelDisplay::DisplayCustomLabelsOrValues, L"custom-labels-or-values" },
                { AxisLabelDisplay::DisplayOnlyCustomLabels, L"only-custom-labels" },
                { AxisLabelDisplay::DisplayCustomLabelsAndValues, L"custom-labels-and-values" },
                { AxisLabelDisplay::NoDisplay, L"no-display" },
                { AxisLabelDisplay::DisplayValues, L"values" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertBracketLineStyleToString(BracketLineStyle value)
            {
            static const std::map<BracketLineStyle, wxString> values = {
                { BracketLineStyle::Arrow, L"arrow" },
                { BracketLineStyle::Lines, L"lines" },
                { BracketLineStyle::ReverseArrow, L"reverse-arrow" },
                { BracketLineStyle::CurlyBraces, L"curly-braces" },
                { BracketLineStyle::NoConnectionLines, L"no-connection-lines" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertLineStyleToString(LineStyle value)
            {
            static const std::map<LineStyle, wxString> values = { { LineStyle::Arrows, L"arrows" },
                                                                  { LineStyle::Lines, L"lines" },
                                                                  { LineStyle::Spline, L"spline" },
                                                                  { LineStyle::Pencil,
                                                                    L"pencil" } };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertResizeMethodToString(ResizeMethod value)
            {
            static const std::map<ResizeMethod, wxString> values = {
                { ResizeMethod::DownscaleOnly, L"downscale-only" },
                { ResizeMethod::DownscaleOrUpscale, L"downscale-or-upscale" },
                { ResizeMethod::UpscaleOnly, L"upscale-only" },
                { ResizeMethod::NoResize, L"no-resize" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertReferenceAreaStyleToString(ReferenceAreaStyle value)
            {
            static const std::map<ReferenceAreaStyle, wxString> values = {
                { ReferenceAreaStyle::FadeFromLeftToRight, L"fade-from-left-to-right" },
                { ReferenceAreaStyle::FadeFromRightToLeft, L"fade-from-right-to-left" },
                { ReferenceAreaStyle::Solid, L"solid" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertAxisTypeToString(AxisType value)
            {
            static const std::map<AxisType, wxString> values = {
                { AxisType::BottomXAxis, L"bottom-x" },
                { AxisType::TopXAxis, L"top-x" },
                { AxisType::LeftYAxis, L"left-y" },
                { AxisType::RightYAxis, L"right-y" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString> ConvertNumberDisplayToString(NumberDisplay value)
            {
            static const std::map<NumberDisplay, wxString> values = {
                { NumberDisplay::Percentage, L"percentage" },
                { NumberDisplay::Value, L"value" },
                { NumberDisplay::Currency, L"currency" },
                { NumberDisplay::ValueSimple, L"value-simple" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxString>
        ConvertReferenceLabelPlacementToString(ReferenceLabelPlacement value)
            {
            static const std::map<ReferenceLabelPlacement, wxString> values = {
                { ReferenceLabelPlacement::OppositeAxis, L"opposite-axis" },
                { ReferenceLabelPlacement::Legend, L"legend" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a PieSliceEffect enum to its JSON string representation.
        /// @param value The pie slice effect enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertPieSliceEffectToString(PieSliceEffect value)
            {
            static const std::map<PieSliceEffect, wxString> values = {
                { PieSliceEffect::Image, L"image" }, { PieSliceEffect::Solid, L"solid" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a PieStyle enum to its JSON string representation.
        /// @param value The pie style enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertPieStyleToString(PieStyle value)
            {
            static const std::map<PieStyle, wxString> values = {
                { PieStyle::None, L"none" },
                { PieStyle::Clockface, L"clockface" },
                { PieStyle::CheesePizza, L"cheese-pizza" },
                { PieStyle::PepperoniPizza, L"pepperoni-pizza" },
                { PieStyle::HawaiianPizza, L"hawaiian-pizza" },
                { PieStyle::CoffeeRing, L"coffee-ring" },
                { PieStyle::Venus, L"venus" },
                { PieStyle::Mars, L"mars" },
                { PieStyle::ChocolateChipCookie, L"chocolate-chip-cookie" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a BoxEffect enum to its JSON string representation.
        /// @param value The box effect enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertBoxEffectToString(BoxEffect value)
            {
            static const std::map<BoxEffect, wxString> values = {
                { BoxEffect::CommonImage, L"common-image" },
                { BoxEffect::Image, L"image" },
                { BoxEffect::FadeFromBottomToTop, L"fade-from-bottom-to-top" },
                { BoxEffect::FadeFromLeftToRight, L"fade-from-left-to-right" },
                { BoxEffect::FadeFromRightToLeft, L"fade-from-right-to-left" },
                { BoxEffect::FadeFromTopToBottom, L"fade-from-top-to-bottom" },
                { BoxEffect::Glassy, L"glassy" },
                { BoxEffect::Solid, L"solid" },
                { BoxEffect::StippleImage, L"stipple-image" },
                { BoxEffect::StippleShape, L"stipple-shape" },
                { BoxEffect::WaterColor, L"watercolor" },
                { BoxEffect::Marker, L"marker" },
                { BoxEffect::Pencil, L"pencil" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a BinningMethod enum to its JSON string representation.
        /// @param value The binning method enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertBinningMethodToString(Graphs::Histogram::BinningMethod value)
            {
            static const std::map<Graphs::Histogram::BinningMethod, wxString> values = {
                { Graphs::Histogram::BinningMethod::BinByIntegerRange, L"bin-by-integer-range" },
                { Graphs::Histogram::BinningMethod::BinByRange, L"bin-by-range" },
                { Graphs::Histogram::BinningMethod::BinUniqueValues, L"bin-unique-values" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts an IntervalDisplay enum to its JSON string representation.
        /// @param value The interval display enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertIntervalDisplayToString(Graphs::Histogram::IntervalDisplay value)
            {
            static const std::map<Graphs::Histogram::IntervalDisplay, wxString> values = {
                { Graphs::Histogram::IntervalDisplay::Cutpoints, L"cutpoints" },
                { Graphs::Histogram::IntervalDisplay::Midpoints, L"midpoints" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a BinLabelDisplay enum to its JSON string representation.
        /// @param value The bin label display enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertBinLabelDisplayToString(BinLabelDisplay value)
            {
            static const std::map<BinLabelDisplay, wxString> values = {
                { BinLabelDisplay::BinPercentage, L"percentage" },
                { BinLabelDisplay::BinValue, L"value" },
                { BinLabelDisplay::BinValueAndPercentage, L"value-and-percentage" },
                { BinLabelDisplay::NoDisplay, L"no-display" },
                { BinLabelDisplay::BinName, L"bin-name" },
                { BinLabelDisplay::BinNameAndValue, L"bin-name-and-value" },
                { BinLabelDisplay::BinNameAndPercentage, L"bin-name-and-percentage" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a LabelPlacement enum to its JSON string representation.
        /// @param value The label placement enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertLabelPlacementToString(LabelPlacement value)
            {
            static const std::map<LabelPlacement, wxString> values = {
                { LabelPlacement::NextToParent, L"next-to-parent" },
                { LabelPlacement::Flush, L"flush" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a LaneSeparatorStyle enum to its JSON string.
        /// @param value The lane separator style enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertLaneSeparatorStyleToString(Graphs::Roadmap::LaneSeparatorStyle value)
            {
            static const std::map<Graphs::Roadmap::LaneSeparatorStyle, wxString> values = {
                { Graphs::Roadmap::LaneSeparatorStyle::SingleLine, L"single-line" },
                { Graphs::Roadmap::LaneSeparatorStyle::DoubleLine, L"double-line" },
                { Graphs::Roadmap::LaneSeparatorStyle::NoDisplay, L"no-display" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a RoadStopTheme enum to its JSON string.
        /// @param value The road stop theme enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertRoadStopThemeToString(Graphs::Roadmap::RoadStopTheme value)
            {
            static const std::map<Graphs::Roadmap::RoadStopTheme, wxString> values = {
                { Graphs::Roadmap::RoadStopTheme::LocationMarkers, L"location-markers" },
                { Graphs::Roadmap::RoadStopTheme::RoadSigns, L"road-signs" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a MarkerLabelDisplay enum to its JSON string.
        /// @param value The marker label display enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertMarkerLabelDisplayToString(Graphs::Roadmap::MarkerLabelDisplay value)
            {
            static const std::map<Graphs::Roadmap::MarkerLabelDisplay, wxString> values = {
                { Graphs::Roadmap::MarkerLabelDisplay::Name, L"name" },
                { Graphs::Roadmap::MarkerLabelDisplay::NameAndAbsoluteValue,
                  L"name-and-absolute-value" },
                { Graphs::Roadmap::MarkerLabelDisplay::NameAndValue, L"name-and-value" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a CandlestickPlot::PlotType enum to its JSON string.
        /// @param value The plot type enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertCandlestickPlotTypeToString(Graphs::CandlestickPlot::PlotType value)
            {
            static const std::map<Graphs::CandlestickPlot::PlotType, wxString> values = {
                { Graphs::CandlestickPlot::PlotType::Candlestick, L"candlestick" },
                { Graphs::CandlestickPlot::PlotType::Ohlc, L"ohlc" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a GraphColumnHeader enum to its JSON string.
        /// @param value The graph column header enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertGraphColumnHeaderToString(GraphColumnHeader value)
            {
            static const std::map<GraphColumnHeader, wxString> values = {
                { GraphColumnHeader::AsHeader, L"as-header" },
                { GraphColumnHeader::AsFooter, L"as-footer" },
                { GraphColumnHeader::NoDisplay, L"no-display" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a FlowShape enum to its JSON string.
        /// @param value The flow shape enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertFlowShapeToString(FlowShape value)
            {
            static const std::map<FlowShape, wxString> values = {
                { FlowShape::Curvy, L"curvy" }, { FlowShape::Jagged, L"jagged" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a DateInterval enum to its JSON string.
        /// @brief Converts a LikertSurveyQuestionFormat to its JSON string.
        /// @param value The survey format enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertLikertSurveyQuestionFormatToString(
            Graphs::LikertChart::LikertSurveyQuestionFormat value)
            {
            using LF = Graphs::LikertChart::LikertSurveyQuestionFormat;
            static const std::map<LF, wxString> values = {
                { LF::TwoPoint, L"two-point" },
                { LF::TwoPointCategorized, L"two-point-categorized" },
                { LF::ThreePoint, L"three-point" },
                { LF::ThreePointCategorized, L"threepoint-categorized" },
                { LF::FourPoint, L"four-point" },
                { LF::FourPointCategorized, L"four-point-categorized" },
                { LF::FivePoint, L"five-point" },
                { LF::FivePointCategorized, L"five-point-categorized" },
                { LF::SixPoint, L"six-point" },
                { LF::SixPointCategorized, L"six-point-categorized" },
                { LF::SevenPoint, L"seven-point" },
                { LF::SevenPointCategorized, L"seven-point-categorized" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @param value The date interval enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertDateIntervalToString(DateInterval value)
            {
            static const std::map<DateInterval, wxString> values = {
                { DateInterval::Daily, L"daily" },
                { DateInterval::FiscalQuarterly, L"fiscal-quarterly" },
                { DateInterval::Monthly, L"monthly" },
                { DateInterval::Weekly, L"weekly" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a FiscalYear enum to its JSON string.
        /// @param value The fiscal year enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertFiscalYearToString(FiscalYear value)
            {
            static const std::map<FiscalYear, wxString> values = {
                { FiscalYear::Education, L"education" }, { FiscalYear::USBusiness, L"us-business" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a TaskLabelDisplay enum to its JSON string.
        /// @param value The task label display enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString>
        ConvertTaskLabelDisplayToString(Graphs::GanttChart::TaskLabelDisplay value)
            {
            static const std::map<Graphs::GanttChart::TaskLabelDisplay, wxString> values = {
                { Graphs::GanttChart::TaskLabelDisplay::Days, L"days" },
                { Graphs::GanttChart::TaskLabelDisplay::Description, L"description" },
                { Graphs::GanttChart::TaskLabelDisplay::DescriptionAndDays,
                  L"description-and-days" },
                { Graphs::GanttChart::TaskLabelDisplay::NoDisplay, L"no-display" },
                { Graphs::GanttChart::TaskLabelDisplay::Resource, L"resource" },
                { Graphs::GanttChart::TaskLabelDisplay::ResourceAndDays, L"resource-and-days" },
                { Graphs::GanttChart::TaskLabelDisplay::ResourceAndDescription,
                  L"resource-and-description" },
                { Graphs::GanttChart::TaskLabelDisplay::ResourceDescriptionAndDays,
                  L"resource-description-and-days" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a RoundingMethod enum to its JSON string representation.
        /// @param value The rounding method enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertRoundingMethodToString(RoundingMethod value)
            {
            static const std::map<RoundingMethod, wxString> values = {
                { RoundingMethod::NoRounding, L"no-rounding" },
                { RoundingMethod::Round, L"round" },
                { RoundingMethod::RoundDown, L"round-down" },
                { RoundingMethod::RoundUp, L"round-up" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a Perimeter enum to its JSON string representation.
        /// @param value The perimeter enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertPerimeterToString(Perimeter value)
            {
            static const std::map<Perimeter, wxString> values = { { Perimeter::Inner, L"inner" },
                                                                  { Perimeter::Outer, L"outer" } };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a Gender enum to its JSON string representation.
        /// @param value The gender enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertGenderToString(Gender value)
            {
            static const std::map<Gender, wxString> values = { { Gender::Female, L"female" },
                                                               { Gender::Male, L"male" } };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a HairStyle enum to its JSON string representation.
        /// @param value The hair style enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertHairStyleToString(HairStyle value)
            {
            static const std::map<HairStyle, wxString> values = {
                { HairStyle::Bald, L"bald" },
                { HairStyle::Bob, L"bob" },
                { HairStyle::Pixie, L"pixie" },
                { HairStyle::Bun, L"bun" },
                { HairStyle::LongStraight, L"long-straight" },
                { HairStyle::HighTopFade, L"high-top-fade" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }

        /// @brief Converts a FacialHair enum to its JSON string representation.
        /// @param value The facial hair enum value.
        /// @returns The string if found, or std::nullopt.
        [[nodiscard]]
        static std::optional<wxString> ConvertFacialHairToString(FacialHair value)
            {
            static const std::map<FacialHair, wxString> values = {
                { FacialHair::CleanShaven, L"clean-shaven" },
                { FacialHair::FiveOClockShadow, L"five-o-clock-shadow" }
            };

            const auto foundValue = values.find(value);
            return (foundValue != values.cend()) ? std::optional<wxString>{ foundValue->second } :
                                                   std::nullopt;
            }
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_ENUM_CONVERT_H
